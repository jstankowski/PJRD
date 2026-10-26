/*
    SPDX-FileCopyrightText: 2020-2026 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Encoder.h"
#include "xJPEG_Transform.h"
#include "xJPEG_TransformConstants.h"
#include "xJPEG_Scan.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Fwd/Inv transform + Quant/InvScale
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xFwdTransformPic(int16* CoeffsTransV[], const xPicYUV* Picture)
{
  const uint16* CmpPtrV   [] = {Picture->getAddr  (eCmp::LM), Picture->getAddr  (eCmp::CB), Picture->getAddr  (eCmp::CR), nullptr};
  const int32   CmpStrideV[] = {Picture->getStride(eCmp::LM), Picture->getStride(eCmp::CB), Picture->getStride(eCmp::CR),       0};

  for(int32 MCU_RowIdx = 0; MCU_RowIdx < m_NumMCUsInHeight; MCU_RowIdx++) //loop over MCUs rows
  {
    const int32 MCU_IdxBeg = MCU_RowIdx * m_NumMCUsInWidth;
    const int32 MCU_IdxEnd = MCU_IdxBeg + m_NumMCUsInWidth;
    m_ThPI.storeTask([this, &CoeffsTransV, &CmpPtrV, &CmpStrideV, MCU_IdxBeg, MCU_IdxEnd](int32 /*ThIdx*/) { xFwdTransformSlc(CoeffsTransV, CmpPtrV, CmpStrideV, MCU_IdxBeg, MCU_IdxEnd); });
  }
  m_ThPI.executeStoredTasks();
}
void xAdvancedEncoder::xFwdTransformSlc(int16* CoeffsTransV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_IdxBeg, int32 MCU_IdxEnd)
{
  //fmt::print("MCU_IdxBeg={}\n", MCU_IdxBeg);
  for(int32 MCU_Idx = MCU_IdxBeg; MCU_Idx < MCU_IdxEnd; MCU_Idx++) //loop over MCUs
  {
    xFwdTransformMCU(CoeffsTransV, CmpPtrV, CmpStrideV, MCU_Idx);
  }
}
void xAdvancedEncoder::xFwdTransformMCU(int16* CoeffsTransV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx)
{
  //calculate MCU position
  const int32 MCU_PosV = MCU_Idx / m_NumMCUsInWidth;
  const int32 MCU_PosH = MCU_Idx % m_NumMCUsInWidth;

  //org samples buffer
  PMBB_ALIGN_JPEG_BLK uint16 SamplesOrg[c_BA];

  //transform blocks
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32 MCU_PelPosV = MCU_PosV << (2 + m_SampFactorVer[CmpIdx]);
    const int32 MCU_PelPosH = MCU_PosH << (2 + m_SampFactorHor[CmpIdx]);

    const uint16* CmpPtr    = CmpPtrV   [CmpIdx];
    const int32   CmpStride = CmpStrideV[CmpIdx];

    int32 BlockIdx = MCU_Idx * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
    for(int32 V = 0; V < m_SampFactorVer[CmpIdx]; V++)
    {
      const int32 BlockPosV = MCU_PelPosV + V * c_BS;
      const int32 BlockResV = m_CmpHeight[CmpIdx] - BlockPosV;
      for(int32 H = 0; H < m_SampFactorHor[CmpIdx]; H++)
      {        
        const int32   BlockPosH = MCU_PelPosH + H * c_BS;
        const int32   BlockResH = m_CmpWidth[CmpIdx] - BlockPosH;
        const uint16* BlockPtr  = CmpPtr + BlockPosV * CmpStride + BlockPosH;     
        if     (BlockResV >= 8 && BlockResH >= 8) { loadEntireBlock(SamplesOrg, BlockPtr, CmpStride); } //C++20 TODO use [[likely]]
        else if(BlockResV >  0 && BlockResH >  0) { loadExtendBlock(SamplesOrg, BlockPtr, CmpStride, xMin(BlockResH, c_BS), xMin(BlockResV, c_BS)); }
        else                                      { zeroEntireBlock(SamplesOrg); }

        const int32 CoeffTransOffset = BlockIdx << c_L2BA;
        xTransform::FwdTransformDCT_8x8(CoeffsTransV[CmpIdx] + CoeffTransOffset, SamplesOrg);
        CoeffsTransV[CmpIdx][CoeffTransOffset] -= xTransformConstants::c_FwdDcCorr; //DC correction - JPEG requires 128 to be subtracted from every input sample - could be done be DC -= 
        BlockIdx++;
      }
    }
  }
}
void xAdvancedEncoder::xInvTransformPic(xPicYUV* Picture, const int16* CoeffsTransV[])
{
        uint16* CmpPtrV   [] = {Picture->getAddr  (eCmp::LM), Picture->getAddr  (eCmp::CB), Picture->getAddr  (eCmp::CR), nullptr};
  const int32   CmpStrideV[] = {Picture->getStride(eCmp::LM), Picture->getStride(eCmp::CB), Picture->getStride(eCmp::CR),       0};

  for(int32 MCU_RowIdx = 0; MCU_RowIdx < m_NumMCUsInHeight; MCU_RowIdx++) //loop over MCUs rows
  {
    const int32 MCU_IdxBeg = MCU_RowIdx * m_NumMCUsInWidth;
    const int32 MCU_IdxEnd = MCU_IdxBeg + m_NumMCUsInWidth;
    m_ThPI.storeTask([this, &CmpPtrV, &CmpStrideV, &CoeffsTransV, MCU_IdxBeg, MCU_IdxEnd](int32 /*ThIdx*/) { xInvTransformSlc(CmpPtrV, CmpStrideV, CoeffsTransV, MCU_IdxBeg, MCU_IdxEnd); });
  }
  m_ThPI.executeStoredTasks();
}
void xAdvancedEncoder::xInvTransformSlc(uint16* CmpPtrV[], const int32 CmpStrideV[], const int16* CoeffsTransV[], int32 MCU_IdxBeg, int32 MCU_IdxEnd)
{
  for(int32 MCU_Idx = MCU_IdxBeg; MCU_Idx < MCU_IdxEnd; MCU_Idx++) //loop over MCUs
  {
    xInvTransformMCU(CmpPtrV, CmpStrideV, CoeffsTransV, MCU_Idx);
  }
}
void xAdvancedEncoder::xInvTransformMCU(uint16* CmpPtrV[], const int32 CmpStrideV[], const int16* CoeffsTransV[], int32 MCU_Idx)
{
  //calculate MCU position
  const int32 MCU_PosV = MCU_Idx / m_NumMCUsInWidth;
  const int32 MCU_PosH = MCU_Idx % m_NumMCUsInWidth;

  //org samples buffer
  PMBB_ALIGN_JPEG_BLK int16  CoeffsTrans[c_BA];
  PMBB_ALIGN_JPEG_BLK uint16 SamplesRec [c_BA];

  //transform blocks
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32 MCU_PelPosV = MCU_PosV << (2 + m_SampFactorVer[CmpIdx]);
    const int32 MCU_PelPosH = MCU_PosH << (2 + m_SampFactorHor[CmpIdx]);

          uint16* CmpPtr    = CmpPtrV   [CmpIdx];
    const int32   CmpStride = CmpStrideV[CmpIdx];

    int32 BlockIdx = MCU_Idx * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
    for(int32 V = 0; V < m_SampFactorVer[CmpIdx]; V++)
    {
      const int32 BlockPosV = MCU_PelPosV + V * c_BS;
      const int32 BlockResV = m_CmpHeight[CmpIdx] - BlockPosV;
      for(int32 H = 0; H < m_SampFactorHor[CmpIdx]; H++)
      {
        const int32 BlockPosH = MCU_PelPosH + H * c_BS;
        const int32 BlockResH = m_CmpWidth[CmpIdx] - BlockPosH;
        uint16* restrict BlockPtr = CmpPtr + BlockPosV * CmpStride + BlockPosH;
        const int32 CoeffTransOffset = BlockIdx << c_L2BA;

        memcpy(CoeffsTrans, CoeffsTransV[CmpIdx] + CoeffTransOffset, c_BA * sizeof(int16));
        CoeffsTrans[0] += xTransformConstants::c_InvDcCorr; //DC correction - JPEG requires 128 to be subtracted from every input sample - could be done be DC -= 
        xTransform::InvTransformDCT_8x8(SamplesRec, CoeffsTrans);

        if     (BlockResV >= 8 && BlockResH >= 8) { storeEntireBlock (BlockPtr, SamplesRec, CmpStride); } //C++20 TODO use [[likely]]
        else if(BlockResV >  0 && BlockResH >  0) { storePartialBlock(BlockPtr, SamplesRec, CmpStride, xMin(BlockResH, c_BS), xMin(BlockResV, c_BS)); }
        else                                      { /* do nothing */ }

        BlockIdx++;
      }
    }
  }
}
void xAdvancedEncoder::xFwdQuantScanPic(int16* CoeffsQuantScanV[], const int16* CoeffsTransV[], const xQuantizerSet& Quant)
{
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32       QuantTabIdx       = m_SOF.getQuantTableId(eCmp(CmpIdx));
    const int32       NumBlocksInWidth  = m_NumBlocksInWidth [CmpIdx];
    const int32       NumBlocksInHeight = m_NumBlocksInHeight[CmpIdx];
    const xQuantizer* Quantizer         = &Quant.getQuantizer(QuantTabIdx);

    int16*       CoeffsQuantScan = CoeffsQuantScanV[CmpIdx];
    const int16* CoeffsTrans     = CoeffsTransV    [CmpIdx];

    for(int32 BlockRowIdx = 0; BlockRowIdx < NumBlocksInHeight; BlockRowIdx++) //loop over block rows
    {
      const int32 BlockIdxBeg = BlockRowIdx * NumBlocksInWidth;
      const int32 BlockIdxEnd = BlockIdxBeg + NumBlocksInWidth;
      m_ThPI.storeTask([CoeffsQuantScan, CoeffsTrans, Quantizer, BlockIdxBeg, BlockIdxEnd](int32 /*ThIdx*/) { xFwdQuantScanRng(CoeffsQuantScan, CoeffsTrans, *Quantizer, BlockIdxBeg, BlockIdxEnd); });
    }
  }
  m_ThPI.executeStoredTasks();
}
void xAdvancedEncoder::xFwdQuantScanRng(int16* CoeffQuantScan, const int16* CoeffTrans, const xQuantizer& Quant, int32 BlockIdxBeg, int32 BlockIdxEnd)
{
  PMBB_ALIGN_JPEG_BLK int16 CoeffQuant[c_BA];

  for(int32 BlockIdx = BlockIdxBeg; BlockIdx < BlockIdxEnd; BlockIdx++)
  {
    const int32 BlockOffset = BlockIdx << c_L2BA;
    Quant.QuantScale(CoeffQuant, CoeffTrans + BlockOffset);
    xScan::Scan(CoeffQuantScan + BlockOffset, CoeffQuant);
  }
}
void xAdvancedEncoder::xInvScanQuantPic(int16* CoeffsTransV[], const int16* CoeffsQuantScanV[], const xQuantizerSet& Quant)
{
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32       QuantTabIdx       = m_SOF.getQuantTableId(eCmp(CmpIdx));
    const int32       NumBlocksInWidth  = m_NumBlocksInWidth [CmpIdx];
    const int32       NumBlocksInHeight = m_NumBlocksInHeight[CmpIdx];
    const xQuantizer* Quantizer         = &Quant.getQuantizer(QuantTabIdx);

    int16*       CoeffsTrans     = CoeffsTransV    [CmpIdx];
    const int16* CoeffsQuantScan = CoeffsQuantScanV[CmpIdx];

    for(int32 BlockRowIdx = 0; BlockRowIdx < NumBlocksInHeight; BlockRowIdx++) //loop over block rows
    {
      const int32 BlockIdxBeg = BlockRowIdx * NumBlocksInWidth;
      const int32 BlockIdxEnd = BlockIdxBeg + NumBlocksInWidth;

      m_ThPI.storeTask([CoeffsTrans, CoeffsQuantScan, Quantizer, BlockIdxBeg, BlockIdxEnd](int32 /*ThIdx*/) {xInvScanQuantRng(CoeffsTrans, CoeffsQuantScan, *Quantizer, BlockIdxBeg, BlockIdxEnd); });
    }
  }
  m_ThPI.executeStoredTasks();
}
void xAdvancedEncoder::xInvScanQuantRng(int16* CoeffTrans, const int16* CoeffQuantScan, const xQuantizer& Quant, int32 BlockIdxBeg, int32 BlockIdxEnd)
{
  PMBB_ALIGN_JPEG_BLK int16 CoeffQuant[c_BA];

  for(int32 BlockIdx = BlockIdxBeg; BlockIdx < BlockIdxEnd; BlockIdx++)
  {
    const int32 BlockOffset = BlockIdx << c_L2BA;
    xScan::InvScan(CoeffQuant, CoeffQuantScan + BlockOffset);
    Quant.InvScale(CoeffTrans + BlockOffset, CoeffQuant);
  }
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG