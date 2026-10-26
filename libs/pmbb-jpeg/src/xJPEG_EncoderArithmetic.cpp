/*
    SPDX-FileCopyrightText: 2020-2026 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Encoder.h"
#include "xJPEG_EntropyUtils.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Lambda estimation
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int64V4 xAdvancedEncoder::xCalcBitsAritPic(const int16* CoeffsQuantScanV[])
{  
  int64V4 TotalEstNumBits;
  if(m_RestartInterval == 0) //no division - serial processing
  {
    TotalEstNumBits = xCalcBitsAritSlc(CoeffsQuantScanV, 0, 0, m_NumMCUsInArea - 1);
  }
  else //divide picture into independent slices
  {
    std::vector<int64V4> EstNumBits;
    EstNumBits.resize(m_NumOfSlices, xMakeVec4<int64>(0));
    for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++) //loop over slices
    {
      const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
      const int32 MCU_IdxLast  = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_NumMCUsInSlice) - 1;
      m_ThPI.storeTask([this, &EstNumBits, SliceIdx, &CoeffsQuantScanV, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/)
        { 
          EstNumBits[SliceIdx] = xCalcBitsAritSlc(CoeffsQuantScanV, SliceIdx, MCU_IdxFirst, MCU_IdxLast);
        });
    }
    m_ThPI.executeStoredTasks();
    TotalEstNumBits = std::accumulate(EstNumBits.cbegin(), EstNumBits.cend(), xMakeVec4<int64>(0));
  }
  return TotalEstNumBits;
}
int64V4 xAdvancedEncoder::xCalcBitsAritSlc(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  m_EntropyAritEsts[SliceIdx].StartSlice();
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++) //loop over MCUs
  {
    xCalcBitsAritMCU(CoeffsQuantScanV, SliceIdx, MCU_Idx);
  }
  m_EntropyAritEsts[SliceIdx].FinishSlice();
  int64V4 EstNumBits = m_EntropyAritEsts[SliceIdx].getNumBits();
  return EstNumBits;
}
void xAdvancedEncoder::xCalcBitsAritMCU(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_Idx)
{
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    int32 EntropyIdDC = m_SOS.getEntropyIdDC(eCmp(CmpIdx));
    int32 EntropyIdAC = m_SOS.getEntropyIdAC(eCmp(CmpIdx));
    int32 BlockIdx    = MCU_Idx * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
    
    for(int32 V = 0; V < m_SampFactorVer[CmpIdx]; V++)
    {
      for(int32 H = 0; H < m_SampFactorHor[CmpIdx]; H++)
      {
        const int32 CoeffOffset = BlockIdx << c_L2BA;
        m_EntropyAritEsts[SliceIdx].EncodeBlock(CoeffsQuantScanV[CmpIdx] + CoeffOffset, (eCmp)CmpIdx, EntropyIdDC, EntropyIdAC);
        BlockIdx++;                                                                                 
      }
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// RDOQ
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xOptQuantAritPic(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture)
{
  if(m_RestartInterval == 0)
  {
    xOptQuantAritSlc(OptCoeffsQuantScanV, CoeffsQuantScanV, CoeffsTransOrgV, Picture, 0);
  }
  else
  {
    for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++)
    {
      m_ThPI.storeTask([this, &OptCoeffsQuantScanV, &CoeffsQuantScanV, &CoeffsTransOrgV, &Picture, SliceIdx](int32 /*ThIdx*/)
        {
          xOptQuantAritSlc(OptCoeffsQuantScanV, CoeffsQuantScanV, CoeffsTransOrgV, Picture, SliceIdx);
        });
    }
    m_ThPI.executeStoredTasks();
  }
}
void xAdvancedEncoder::xOptQuantAritSlc(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture, int32 SliceIdx)
{
  const uint16* CmpPtrV   [] = {Picture->getAddr  (eCmp::LM), Picture->getAddr  (eCmp::CB), Picture->getAddr  (eCmp::CR), nullptr};
  const int32   CmpStrideV[] = {Picture->getStride(eCmp::LM), Picture->getStride(eCmp::CB), Picture->getStride(eCmp::CR),       0};

  m_EntropyAritEsts[SliceIdx].StartSlice();

  //loop over MCUs
  const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
  const int32 MCU_IdxLast  = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_NumMCUsInSlice) - 1;
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++)
  {
    xOptQuantAritMCU(OptCoeffsQuantScanV, CoeffsQuantScanV, CoeffsTransOrgV, CmpPtrV, CmpStrideV, SliceIdx, MCU_Idx);
  }

  m_EntropyAritEsts[SliceIdx].FinishSlice();
}
void xAdvancedEncoder::xOptQuantAritMCU(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 SliceIdx, int32 MCU_Idx)
{
  //calculate MCU position
  int32 MCU_PosV = MCU_Idx / m_NumMCUsInWidth;
  int32 MCU_PosH = MCU_Idx % m_NumMCUsInWidth;

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
        const int32 BlockPosH = MCU_PelPosH + H * c_BS;
        const int32 BlockResH = m_CmpWidth[CmpIdx] - BlockPosH;

        const uint16* BlockPtr = CmpPtr + BlockPosV * CmpStride + BlockPosH;          

        if     (BlockResV >= 8 && BlockResH >= 8) { loadEntireBlock(SamplesOrg, BlockPtr, CmpStride); } //C++20 TODO use [[likely]]
        else if(BlockResV >  0 && BlockResH >  0) { loadExtendBlock(SamplesOrg, BlockPtr, CmpStride, xMin(BlockResH, c_BS), xMin(BlockResV, c_BS)); }
        else                                      { zeroEntireBlock(SamplesOrg); }

        const int32 CoeffOffset = BlockIdx << c_L2BA;
        xOptQuantAritBLK (OptCoeffsQuantScanV[CmpIdx] + CoeffOffset, CoeffsQuantScanV[CmpIdx] + CoeffOffset, CoeffsTransOrgV[CmpIdx] + CoeffOffset, SamplesOrg, SliceIdx, eCmp(CmpIdx));
        BlockIdx++;
      }
    }    
  }
}
void xAdvancedEncoder::xOptQuantAritBLK(int16* OptCoeffQuantScan, const int16* CoeffsQuantScan, const int16* CoeffsTransOrg, const uint16* SamplesOrg, int32 SliceIdx, eCmp CmpId)
{
  const int32       QuantTabId  = m_SOF.getQuantTableId(CmpId);
  const int32       EntropyIdDC = m_SOS.getEntropyIdDC (CmpId);
  const int32       EntropyIdAC = m_SOS.getEntropyIdAC (CmpId);
  const flt64       LambdaFrac  = m_Lambda[(int32)CmpId] / (1 << xArithmCoreCmn::c_FracBitsShift);
  const xQuantizer& Quantizer   = m_QuantMain.getQuantizer(QuantTabId);

  int32 LastNonZero = xEntropyUtils::findLastNonZero(CoeffsQuantScan);
  if(LastNonZero == 0) { memcpy(OptCoeffQuantScan, CoeffsQuantScan, c_BA * sizeof(int16)); return; } //only DC - nothing to do here

  xEntropyAritEstStateAC EntropyAritEstStateAC;

  const int64 OrgFracBitsDC = m_EntropyAritEsts[SliceIdx].EncodeAndGetFracBitsDC(CoeffsQuantScan[0], CmpId, EntropyIdDC);
  m_EntropyAritEsts[SliceIdx].storeStateAC(EntropyAritEstStateAC, EntropyIdAC);
  const int64 OrgFracBitsAC = m_EntropyAritEsts[SliceIdx].EncodeAndGetFracBitsAC(CoeffsQuantScan, EntropyIdAC);
  m_EntropyAritEsts[SliceIdx].loadStateAC(EntropyAritEstStateAC, EntropyIdAC);

  int64  BestFracBits = OrgFracBitsDC + OrgFracBitsAC;
  uint64 BestDist     = xCalcDistBLK(CoeffsQuantScan, CoeffsTransOrg, SamplesOrg, Quantizer);
  flt64  BestCost     = (flt64)BestDist + LambdaFrac * (flt64)BestFracBits;

  PMBB_ALIGN_JPEG_BLK int16 TmpCoeffsQuantScan[c_BA];
  memcpy(TmpCoeffsQuantScan, CoeffsQuantScan, c_BA * sizeof(int16));

  for (int32 PassIdx = 0; PassIdx < m_NumOptPassesBlock; PassIdx++)
  {    
    for (int32 i = LastNonZero; i >= 1; i--)
    {
      const int16 OrgCoeff = TmpCoeffsQuantScan[i];
      if (!m_ProcessZeroCoeffs && OrgCoeff == 0) { continue; }
      
      int16 BestCoeff = TmpCoeffsQuantScan[i];
      //try zero
      if(OrgCoeff != 0)
      {
        TmpCoeffsQuantScan[i] = 0;
        int64  CurrFracBits = OrgFracBitsDC + m_EntropyAritEsts[SliceIdx].EncodeAndGetFracBitsAC(TmpCoeffsQuantScan, EntropyIdAC);
        m_EntropyAritEsts[SliceIdx].loadStateAC(EntropyAritEstStateAC, EntropyIdAC);
        uint64 CurrDist = xCalcDistBLK(TmpCoeffsQuantScan, CoeffsTransOrg, SamplesOrg, Quantizer);
        flt64  CurrCost = (flt64)CurrDist + LambdaFrac * (flt64)CurrFracBits;
        if (CurrCost < BestCost)
        {
          BestFracBits = CurrFracBits;
          BestCost     = CurrCost;
          BestCoeff    = 0;
        }
      }

      //try +1
      if(OrgCoeff != -1)
      {
        TmpCoeffsQuantScan[i] = OrgCoeff + 1;
        int64  CurrFracBits = OrgFracBitsDC + m_EntropyAritEsts[SliceIdx].EncodeAndGetFracBitsAC(TmpCoeffsQuantScan, EntropyIdAC);
        m_EntropyAritEsts[SliceIdx].loadStateAC(EntropyAritEstStateAC, EntropyIdAC);
        uint64 CurrDist = xCalcDistBLK(TmpCoeffsQuantScan, CoeffsTransOrg, SamplesOrg, Quantizer);
        flt64  CurrCost = (flt64)CurrDist + LambdaFrac * (flt64)CurrFracBits;
        if (CurrCost < BestCost)
        {
          BestFracBits = CurrFracBits;
          BestCost     = CurrCost;
          BestCoeff    = OrgCoeff + 1;
        }
      }

      //try -1  
      if(OrgCoeff != 1)
      {
        TmpCoeffsQuantScan[i] = OrgCoeff - 1;
        int64  CurrFracBits = OrgFracBitsDC + m_EntropyAritEsts[SliceIdx].EncodeAndGetFracBitsAC(TmpCoeffsQuantScan, EntropyIdAC);
        m_EntropyAritEsts[SliceIdx].loadStateAC(EntropyAritEstStateAC, EntropyIdAC);
        uint64 CurrDist = xCalcDistBLK(TmpCoeffsQuantScan, CoeffsTransOrg, SamplesOrg, Quantizer);
        flt64  CurrCost = (flt64)CurrDist + LambdaFrac * (flt64)CurrFracBits;
        if (CurrCost < BestCost)
        {
          BestFracBits = CurrFracBits;
          BestCost     = CurrCost;
          BestCoeff    = OrgCoeff - 1;
        }
      }

      TmpCoeffsQuantScan[i] = BestCoeff;      
    }
  }

  m_EntropyAritEsts[SliceIdx].EncodeAndGetFracBitsAC(TmpCoeffsQuantScan, EntropyIdAC);
  memcpy(OptCoeffQuantScan, TmpCoeffsQuantScan, c_BA * sizeof(int16));
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Arithmetic conditioning optimization
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xOptAritPic(const int16* CoeffsQuantScanV[])
{
  std::vector<int64V4> BitsDC(256, xMakeVec4<int64>(std::numeric_limits<int64>::max())); //[(m_U << 4) + m_L]
  std::vector<int64V4> BitsAC( 64, xMakeVec4<int64>(std::numeric_limits<int64>::max())); //[Kx]

  xCalcAritCondDC(BitsDC, CoeffsQuantScanV);
  xCalcAritCondAC(BitsAC, CoeffsQuantScanV);

  //select DC
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    int64 BestBits = std::numeric_limits<int64>::max();
    int32 BestU    = NOT_VALID;
    int32 BestL    = NOT_VALID;

    for(int32 U = 0; U <= 15; U++)
    {
      for(int32 L = 0; L <= U; L++)
      {
        int64 TotalBits = BitsDC[(U << 4) + L][CmpIdx];
        if(TotalBits < BestBits) { BestBits = TotalBits; BestU = U; BestL = L; }
        //if(U == xJPEG_Constants::c_DefaultU && L == xJPEG_Constants::c_DefaultL) { fmt::print("DEFAULT Cmp={} U={} L={} TotalBits={}\n", CmpIdx, U, L, TotalBits); }
      }
    }
    //fmt::print("RESULT  Cmp={} U={} L={} TotalBits={}\n", CmpIdx, BestU, BestL, BestBits);
    m_ACs[CmpIdx << 1].InitCustom((uint8)CmpIdx, xJFIF::xArithCond::tClass::DC, BestU, BestL, NOT_VALID);
  }

  //select AC
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    int64 BestBits = std::numeric_limits<int64>::max();
    int32 BestKx   = NOT_VALID;

    for(int32 Kx = 1; Kx <= 63; Kx++)
    {
      int64 TotalBits = BitsAC[Kx][CmpIdx];
      if(TotalBits < BestBits) { BestBits = TotalBits; BestKx = Kx; }
      //if(Kx == xJPEG_Constants::c_DefaultKx) { fmt::print("DEFAULT Cmp={} Kx={} TotalBits={}\n", CmpIdx, Kx, TotalBits); }
    }
    //fmt::print("RESULT  Cmp={} Kx={} TotalBits={}\n", CmpIdx, BestKx, BestBits);
    m_ACs[(CmpIdx << 1) + 1].InitCustom((uint8)CmpIdx, xJFIF::xArithCond::tClass::AC, NOT_VALID, NOT_VALID, BestKx);
  }
  
  if(m_NumOptPassesPic > 1) { for(int32 i = 0; i < (int32)m_EntropyAritEsts.size(); i++) { m_EntropyAritEsts[i].Init(m_ACs); } }
  for(int32 i = 0; i < (int32)m_EntropyAritEncs.size(); i++) { m_EntropyAritEncs[i].Init(m_ACs); }
}
void xAdvancedEncoder::xCalcAritCondDC(std::vector<int64V4>& BitsDC, const int16* CoeffsQuantScanV[])
{
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    for(int32 U = 0; U <= 15; U++)
    {
      for(int32 L = 0; L <= U; L++)
      {
        int64 TotalBits = 0;
        for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++)
        {
          TotalBits += xCalcBitsAritSliceDC(CoeffsQuantScanV, (eCmp)CmpIdx, SliceIdx, U, L);
        }
        BitsDC[(U << 4) + L][CmpIdx] = TotalBits;
      } //L
    } //U
  } //CmpIdx
}
void xAdvancedEncoder::xCalcAritCondAC(std::vector<int64V4>& BitsAC, const int16* CoeffsQuantScanV[])
{
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    for(int32 Kx = 1; Kx <= 63; Kx++)
    {
      int64 TotalBits = 0;
      for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++)
      {
        TotalBits += xCalcBitsAritSliceAC(CoeffsQuantScanV, (eCmp)CmpIdx, SliceIdx, Kx);
      } 
      BitsAC[Kx][CmpIdx] = TotalBits;
    } //Kx
  } //CmpIdx
}
int64 xAdvancedEncoder::xCalcBitsAritSliceDC(const int16* CoeffsQuantScanV[], eCmp Cmp, int32 SliceIdx, int32 U, int32 L)
{
  const int16* CoeffsQuantScanCmp = CoeffsQuantScanV[(int32)Cmp];
  const int32  EntropyIdDC        = m_SOS.getEntropyIdDC(Cmp);

  m_EntropyAritEsts[SliceIdx].InitDC(U, L, EntropyIdDC);
  m_EntropyAritEsts[SliceIdx].StartSlice();
  const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
  const int32 MCU_IdxLast  = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_NumMCUsInSlice) - 1;
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++)
  {
    int32 BlockIdx = MCU_Idx * m_SampFactorVer[(int32)Cmp] * m_SampFactorHor[(int32)Cmp];
    for(int32 V = 0; V < m_SampFactorVer[(int32)Cmp]; V++)
    {
      for(int32 H = 0; H < m_SampFactorHor[(int32)Cmp]; H++)
      {
        const int32 CoeffOffset = BlockIdx << c_L2BA;
        m_EntropyAritEsts[SliceIdx].EncodeDC(*(CoeffsQuantScanCmp + CoeffOffset), Cmp, EntropyIdDC);
      }
    }
  }
  m_EntropyAritEsts[SliceIdx].FinishSlice();
  int64 Bits = m_EntropyAritEsts[SliceIdx].getNumBitsCore();
  return Bits;
}
int64 xAdvancedEncoder::xCalcBitsAritSliceAC(const int16* CoeffsQuantScanV[], eCmp Cmp, int32 SliceIdx, int32 Kx)
{
  const int16* CoeffsQuantScanCmp = CoeffsQuantScanV[(int32)Cmp];
  const int32  EntropyIdAC        = m_SOS.getEntropyIdAC(Cmp);

  m_EntropyAritEsts[SliceIdx].InitAC(Kx, EntropyIdAC);
  m_EntropyAritEsts[SliceIdx].StartSlice();
  const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
  const int32 MCU_IdxLast = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_NumMCUsInSlice) - 1;
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++)
  {
    int32 BlockIdx = MCU_Idx * m_SampFactorVer[(int32)Cmp] * m_SampFactorHor[(int32)Cmp];
    for(int32 V = 0; V < m_SampFactorVer[(int32)Cmp]; V++)
    {
      for(int32 H = 0; H < m_SampFactorHor[(int32)Cmp]; H++)
      {
        const int32 CoeffOffset = BlockIdx << c_L2BA;
        m_EntropyAritEsts[SliceIdx].EncodeAC(CoeffsQuantScanCmp + CoeffOffset, EntropyIdAC);
      }
    }
  }
  m_EntropyAritEsts[SliceIdx].FinishSlice();
  int64 Bits = m_EntropyAritEsts[SliceIdx].getNumBitsCore();
  return Bits;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Huffman & arithmetic encode
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xEncodeAritPic(const int16* CoeffsQuantScanV[])
{
  if(m_RestartInterval == 0) //no division - serial processing
  {
    xEncodeAritSlc(CoeffsQuantScanV, 0, 0, m_NumMCUsInArea - 1);
  }
  else //divide picture into independent slices
  {
    for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++) //loop over slices
    {
      const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
      const int32 MCU_IdxLast = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_RestartInterval) - 1;
      m_ThPI.storeTask([this, &CoeffsQuantScanV, SliceIdx, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/) { xEncodeAritSlc(CoeffsQuantScanV, SliceIdx, MCU_IdxFirst, MCU_IdxLast); });
    }
    m_ThPI.executeStoredTasks();
  }
}
void xAdvancedEncoder::xEncodeAritSlc(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  m_EncBuffers     [SliceIdx].reset();
  m_EntropyAritEncs[SliceIdx].StartSlice(&m_EncBuffers[SliceIdx]);
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++) { xEncodeAritMCU(CoeffsQuantScanV, SliceIdx, MCU_Idx); }
  m_EntropyAritEncs[SliceIdx].FinishSlice();
  m_SliceBuffers   [SliceIdx].reset();
  xJFIF::AddStuffing(&m_SliceBuffers[SliceIdx], &m_EncBuffers[SliceIdx]);
}
void xAdvancedEncoder::xEncodeAritMCU(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_Idx)
{
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    int32 EntropyIdDC = m_SOS.getEntropyIdDC(eCmp(CmpIdx));
    int32 EntropyIdAC = m_SOS.getEntropyIdAC(eCmp(CmpIdx));

    int32 BlockIdx = MCU_Idx * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
    for(int32 V = 0; V < m_SampFactorVer[CmpIdx]; V++)
    {
      for(int32 H = 0; H < m_SampFactorHor[CmpIdx]; H++)
      {
        const int32 CoeffScanOffset = BlockIdx << c_L2BA;        
        m_EntropyAritEncs[SliceIdx].EncodeBlock(CoeffsQuantScanV[CmpIdx] + CoeffScanOffset, eCmp(CmpIdx), EntropyIdDC, EntropyIdAC);
        BlockIdx++;
      }
    }
  }
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG