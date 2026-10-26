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
int64V4 xAdvancedEncoder::xCalcBitsHuffPic(const int16* CoeffsQuantScanV[])
{
  std::vector<int64V4> EstNumBits;
  if(m_RestartInterval == 0) //no division - however, process picture in MCU rows
  {
    EstNumBits.resize(m_NumMCUsInHeight);
    for(int32 MCU_RowIdx = 0; MCU_RowIdx < m_NumMCUsInHeight; MCU_RowIdx++) //loop over MCUs rows
    {
      const int32 MCU_IdxFirst = MCU_RowIdx   * m_NumMCUsInWidth    ;
      const int32 MCU_IdxLast  = MCU_IdxFirst + m_NumMCUsInWidth - 1;
      m_ThPI.storeTask([this, &EstNumBits, MCU_RowIdx, &CoeffsQuantScanV, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/)
        { 
          EstNumBits[MCU_RowIdx] = xCalcBitsHuffSlc(CoeffsQuantScanV, MCU_IdxFirst, MCU_IdxLast);
        });
    }
  }
  else //divide picture into independent slices
  {
    EstNumBits.resize(m_NumOfSlices);
    for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++) //loop over slices
    {
      const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
      const int32 MCU_IdxLast  = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_NumMCUsInSlice) - 1;
      m_ThPI.storeTask([this, &EstNumBits, SliceIdx, &CoeffsQuantScanV, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/)
        { 
          EstNumBits[SliceIdx] = xCalcBitsHuffSlc(CoeffsQuantScanV, MCU_IdxFirst, MCU_IdxLast);
        });
    }
  }
  m_ThPI.executeStoredTasks();

  int64V4 TotalEstNumBits = std::accumulate(EstNumBits.cbegin(), EstNumBits.cend(), xMakeVec4<int64>(0));
  return TotalEstNumBits;
}
int64V4 xAdvancedEncoder::xCalcBitsHuffSlc(const int16* CoeffsQuantScanV[], int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  int64V4 EstNumBits = xMakeVec4<int64>(0);  
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++) //loop over MCUs
  {
    EstNumBits += xCalcBitsHuffMCU(CoeffsQuantScanV, MCU_Idx);
  }
  return EstNumBits;
}
int64V4 xAdvancedEncoder::xCalcBitsHuffMCU(const int16* CoeffsQuantScanV[], int32 MCU_Idx)
{
  const bool MCU_FirstInSlice = m_RestartInterval > 0 ? (MCU_Idx % m_RestartInterval == 0) : MCU_Idx == 0;

  int64V4 EstNumBits = xMakeVec4<int64>(0);

  //estimate blocks
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
        const bool  FirstInSlc  = MCU_FirstInSlice && V == 0 && H == 0;
        const int32 LastDC      = FirstInSlc ? 0 : CoeffsQuantScanV[CmpIdx][CoeffOffset - c_BA];

        EstNumBits[CmpIdx] += m_EntropyHuffEst.EstimateBlock(CoeffsQuantScanV[CmpIdx] + CoeffOffset, LastDC, EntropyIdDC, EntropyIdAC);
        BlockIdx++;                                                                                 
      }
    }
  }
  return EstNumBits;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// RDOQ
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xOptQuantHuffPic(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture)
{
  if(m_RestartInterval == 0) 
  {
    for(int32 MCU_RowIdx = 0; MCU_RowIdx < m_NumMCUsInHeight; MCU_RowIdx++) //loop over MCUs rows
    {
      const int32 MCU_IdxFirst = MCU_RowIdx   * m_NumMCUsInWidth    ;
      const int32 MCU_IdxLast  = MCU_IdxFirst + m_NumMCUsInWidth - 1;
      m_ThPI.storeTask([this, &OptCoeffsQuantScanV, &CoeffsQuantScanV, &CoeffsTransOrgV, &Picture, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/)
        {
          xOptQuantHuffSlc(OptCoeffsQuantScanV, CoeffsQuantScanV, CoeffsTransOrgV, Picture, MCU_IdxFirst, MCU_IdxLast);
        });
    }
  }
  else //divide picture into independent slices
  {
    for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++) //loop over slices
    {
      const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
      const int32 MCU_IdxLast  = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_RestartInterval) - 1;
      m_ThPI.storeTask([this, &OptCoeffsQuantScanV, &CoeffsQuantScanV, &CoeffsTransOrgV, &Picture, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/)
        {
          xOptQuantHuffSlc(OptCoeffsQuantScanV, CoeffsQuantScanV, CoeffsTransOrgV, Picture, MCU_IdxFirst, MCU_IdxLast);
        });
    }
  }
  m_ThPI.executeStoredTasks();
}
void xAdvancedEncoder::xOptQuantHuffSlc(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture, int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  const uint16* CmpPtrV   [] = {Picture->getAddr  (eCmp::LM), Picture->getAddr  (eCmp::CB), Picture->getAddr  (eCmp::CR), nullptr};
  const int32   CmpStrideV[] = {Picture->getStride(eCmp::LM), Picture->getStride(eCmp::CB), Picture->getStride(eCmp::CR),       0};

  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++) //loop over MCUs
  {
    xOptQuantHuffMCU(OptCoeffsQuantScanV, CoeffsQuantScanV, CoeffsTransOrgV, CmpPtrV, CmpStrideV, MCU_Idx);
  }
}
void xAdvancedEncoder::xOptQuantHuffMCU(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx)
{
  //calculate MCU position
  const int32 MCU_PosV         = MCU_Idx / m_NumMCUsInWidth;
  const int32 MCU_PosH         = MCU_Idx % m_NumMCUsInWidth;
  const bool  MCU_FirstInSlice = m_RestartInterval > 0 ? (MCU_Idx % m_RestartInterval == 0) : MCU_Idx == 0;

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
        const bool  FirstInSlc  = MCU_FirstInSlice && V == 0 && H == 0;
        const int32 LastDC      = FirstInSlc ? 0 : CoeffsQuantScanV[CmpIdx][CoeffOffset - c_BA];
        xOptQuantHuffBLK (OptCoeffsQuantScanV[CmpIdx] + CoeffOffset, CoeffsQuantScanV[CmpIdx] + CoeffOffset, CoeffsTransOrgV[CmpIdx] + CoeffOffset, SamplesOrg, eCmp(CmpIdx), LastDC);        
        BlockIdx++;
      }
    }
  }
}
void xAdvancedEncoder::xOptQuantHuffBLK(int16* OptCoeffQuantScan, const int16* CoeffsQuantScan, const int16* CoeffsTransOrg, const uint16* SamplesOrg, eCmp CmpId, int32 LastDC)
{
  const int32 QuantTabId  = m_SOF.getQuantTableId(CmpId);
  const int32 EntropyIdDC = m_SOS.getEntropyIdDC (CmpId);
  const int32 EntropyIdAC = m_SOS.getEntropyIdAC (CmpId);
  const flt64 Lambda      = m_Lambda[(int32)CmpId];

  const xQuantizer& Quantizer = m_QuantMain.getQuantizer(QuantTabId);

  int32 LastNonZero = xEntropyUtils::findLastNonZero(CoeffsQuantScan);
  if(LastNonZero == 0) { memcpy(OptCoeffQuantScan, CoeffsQuantScan, c_BA * sizeof(int16)); return; } //only DC - nothing to do here

  const int32 OrgBitsDC = m_EntropyHuffEst.EstimateBlockDC(CoeffsQuantScan, LastDC, EntropyIdDC);
  const int32 OrgBitsAC = m_EntropyHuffEst.EstimateBlockAC(CoeffsQuantScan,         EntropyIdAC);

  int32  BestBits = OrgBitsDC + OrgBitsAC;
  uint64 BestDist = xCalcDistBLK(CoeffsQuantScan, CoeffsTransOrg, SamplesOrg, Quantizer);
  flt64  BestCost = (flt64)BestDist + Lambda * (flt64)BestBits;

  PMBB_ALIGN_JPEG_BLK int16 TmpCoeffsQuantScan[c_BA];
  memcpy(TmpCoeffsQuantScan, CoeffsQuantScan, c_BA * sizeof(int16));

  for (int32 PassIdx = 0; PassIdx < m_NumOptPassesBlock; PassIdx++)
  {    
    for (int32 i = LastNonZero; i >= 1; i--)
    {
      const int16 OrgCoeff  = TmpCoeffsQuantScan[i];
      if (!m_ProcessZeroCoeffs && OrgCoeff == 0) { continue; }
      
      int16 BestCoeff = TmpCoeffsQuantScan[i];
      //try zero
      if(OrgCoeff != 0)
      {
        TmpCoeffsQuantScan[i] = 0;
        int32  CurrBits = OrgBitsDC + m_EntropyHuffEst.EstimateBlockAC(TmpCoeffsQuantScan, EntropyIdAC);
        uint64 CurrDist = xCalcDistBLK(TmpCoeffsQuantScan, CoeffsTransOrg, SamplesOrg, Quantizer);
        flt64  CurrCost = (double)CurrDist + Lambda * (flt64)CurrBits;
        if (CurrCost < BestCost)
        {
          BestBits  = CurrBits;
          BestCost  = CurrCost;
          BestCoeff = 0;
        }
      }

      //try +1
      if(OrgCoeff != -1)
      {
        TmpCoeffsQuantScan[i] = OrgCoeff + 1;
        int32  CurrBits = OrgBitsDC + m_EntropyHuffEst.EstimateBlockAC(TmpCoeffsQuantScan, EntropyIdAC);
        uint64 CurrDist = xCalcDistBLK(TmpCoeffsQuantScan, CoeffsTransOrg, SamplesOrg, Quantizer);
        flt64  CurrCost = (flt64)CurrDist + Lambda * (flt64)CurrBits;
        if (CurrCost < BestCost)
        {
          BestBits = CurrBits;
          BestCost  = CurrCost;
          BestCoeff = OrgCoeff + 1;
        }
      }

      //try -1  
      if(OrgCoeff != 1)
      {
        TmpCoeffsQuantScan[i] = OrgCoeff - 1;
        int32  CurrBits = OrgBitsDC + m_EntropyHuffEst.EstimateBlockAC(TmpCoeffsQuantScan, EntropyIdAC);
        uint64 CurrDist = xCalcDistBLK(TmpCoeffsQuantScan, CoeffsTransOrg, SamplesOrg, Quantizer);
        flt64  CurrCost = (flt64)CurrDist + Lambda * (flt64)CurrBits;
        if (CurrCost < BestCost)
        {
          BestBits = CurrBits;
          BestCost  = CurrCost;
          BestCoeff = OrgCoeff - 1;
        }
      }

      TmpCoeffsQuantScan[i] = BestCoeff;
    }
  }

  //int32 TestBits = m_EntropyHuffEst.EstimateBlock(TmpCoeffsScan, CmpId, HuffTabIdDC, HuffTabIdAC);
  memcpy(OptCoeffQuantScan, TmpCoeffsQuantScan, c_BA * sizeof(int16));
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Huffman tables optimization
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xOptHuffPic(const int16* CoeffsQuantScanV[])
{
  //calc symbol symbols
  xCntHuffPic(CoeffsQuantScanV);

  //build huffman tables
  std::vector<xJFIF::xHuffTable> NewHT = m_HTs;

  for(xJFIF::xHuffTable& HuffTable : NewHT)
  {
    xJFIF::xHuffTable::tClass HuffTableClass = HuffTable.getClass();
    int32                     HuffTableId    = HuffTable.getIdx  ();
    switch(HuffTableClass)
    {
    case xJFIF::xHuffTable::tClass::DC:
    {
      const uint32* SymbolCount = m_EntropyHuffCnts[0].getSymbolCountDC(HuffTableId);
      uint8 LengthTable[xJPEG_Constants::c_MaxNumCodeSymbolsDC+1];
      xHuffmanTabBuilder::buildLengthTable(LengthTable, SymbolCount, xJPEG_Constants::c_MaxNumCodeSymbolsDC);
      HuffTable.InitCustom((uint8)HuffTableId, HuffTableClass, LengthTable);
      break;
    }
    case xJFIF::xHuffTable::tClass::AC:
    {
      const uint32* SymbolCount = m_EntropyHuffCnts[0].getSymbolCountAC(HuffTableId);
      uint8 LengthTable[xJPEG_Constants::c_MaxNumCodeSymbolsAC+1];
      xHuffmanTabBuilder::buildLengthTable(LengthTable, SymbolCount, xJPEG_Constants::c_MaxNumCodeSymbolsAC);
      HuffTable.InitCustom((uint8)HuffTableId, HuffTableClass, LengthTable);
      break;
    }
    default: break;
    }
  }

  //print tables
  if(m_VerboseLevel >= 8)
  {
    std::string Dump = fmt::format("HuffmanTablesPrev\n");
    for(int32 i = 0; i < (int32)m_HTs.size(); i++) { Dump += fmt::format("  Table_{}\n", i) + m_HTs[i].Format("    "); }
    Dump += fmt::format("HuffmanTablesNext\n");
    for(int32 i = 0; i < (int32)NewHT.size(); i++) { Dump += fmt::format("  Table_{}\n", i) + NewHT[i].Format("    "); }
    fmt::print("{}\n", Dump);
  }

  if(m_VerboseLevel >= 6)
  {
    std::string Dump = fmt::format("HuffmanTablesPrev\n");
    for(xJFIF::xHuffTable& HuffTable : m_HTs)
    {
      const uint32* SymbolCount = m_EntropyHuffCnts[0].getSymbolCount(HuffTable.getClass(), HuffTable.getIdx());
      flt64 AvgCodeLength = xHuffmanTabBuilder::calcAvgCodeLength(HuffTable, SymbolCount);
      Dump += fmt::format("  Table Idx={} Class={} AvgCodeLength={:7.5f}\n", HuffTable.getIdx(), xJFIF::xEntropyTableClass2Str(HuffTable.getClass()), AvgCodeLength);
    }
    Dump += fmt::format("HuffmanTablesNext\n");
    for(xJFIF::xHuffTable& HuffTable : NewHT)
    {
      const uint32* SymbolCount = m_EntropyHuffCnts[0].getSymbolCount(HuffTable.getClass(), HuffTable.getIdx());
      flt64 AvgCodeLength = xHuffmanTabBuilder::calcAvgCodeLength(HuffTable, SymbolCount);
      Dump += fmt::format("  Table Idx={} Class={} AvgCodeLength={:7.5f}\n", HuffTable.getIdx(), xJFIF::xEntropyTableClass2Str(HuffTable.getClass()), AvgCodeLength);
    }
    fmt::print("{}\n", Dump);
  }

  m_HTs = NewHT;
  
  if(m_NumOptPassesPic > 1) { m_EntropyHuffEst.Init(m_HTs); }
  m_EncoderHuffBank.Init(m_HTs);
  for(int32 i = 0; i < (int32)m_EntropyHuffEncs.size(); i++) { m_EntropyHuffEncs[i].SetEncoders(m_EncoderHuffBank); }
}
void xAdvancedEncoder::xCntHuffPic(const int16* CoeffsQuantScanV[])
{
  const int32 NumCounters = (int32)m_EntropyHuffCnts.size();
  for(int32 i = 0; i < NumCounters; i++) { m_EntropyHuffCnts[i].Init(m_HTs); }

  //count symbols
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32  EntropyIdDC       = m_SOS.getEntropyIdDC(eCmp(CmpIdx));
    const int32  EntropyIdAC       = m_SOS.getEntropyIdAC(eCmp(CmpIdx));
    const int32  BlocksPerSlice    = m_RestartInterval * m_NumBlocksInMCU[CmpIdx];
    const int32  NumBlocksInWidth  = m_NumBlocksInWidth [CmpIdx];
    const int32  NumBlocksInHeight = m_NumBlocksInHeight[CmpIdx];
    const int16* CoeffsQuantScan   = CoeffsQuantScanV[CmpIdx];

    for(int32 BlockRowIdx = 0; BlockRowIdx < NumBlocksInHeight; BlockRowIdx++) //loop over block rows
    {
      const int32 BlockIdxBeg = BlockRowIdx * NumBlocksInWidth;
      const int32 BlockIdxEnd = BlockIdxBeg + NumBlocksInWidth;
      m_ThPI.storeTask([this, CoeffsQuantScan, EntropyIdDC, EntropyIdAC, BlockIdxBeg, BlockIdxEnd, BlocksPerSlice](int32 ThIdx) { xCntHuffRng(CoeffsQuantScan, ThIdx, EntropyIdDC, EntropyIdAC, BlockIdxBeg, BlockIdxEnd, BlocksPerSlice); });
    }
  }
  m_ThPI.executeStoredTasks();

  //agregate counters
  for(int32 i = 1; i < NumCounters; i++)
  {
    m_EntropyHuffCnts[0].AddCounters(m_EntropyHuffCnts[i]);
  }
}
void xAdvancedEncoder::xCntHuffRng(const int16* CoeffsQuantScan, int32 CounterIdx, int32 HuffTabIdDC, int32 HuffTabIdAC, int32 BlockIdxBeg, int32 BlockIdxEnd, int32 BlocksPerSlice)
{
  for(int32 BlockIdx = BlockIdxBeg; BlockIdx < BlockIdxEnd; BlockIdx++)
  {
    const int32 CoeffOffset = BlockIdx << c_L2BA;
    const bool  FirstInSlc  = m_RestartInterval > 0 ? (BlockIdx % BlocksPerSlice == 0) : BlockIdx == 0;
    const int32 LastDC      = FirstInSlc ? 0 : CoeffsQuantScan[CoeffOffset - c_BA];
    m_EntropyHuffCnts[CounterIdx].CountBlock(CoeffsQuantScan + CoeffOffset, LastDC, HuffTabIdDC, HuffTabIdAC);
  }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Huffman encode
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xEncodeHuffPic(const int16* CoeffsQuantScanV[])
{
  if(m_RestartInterval == 0) //no division - however, try process picture in MCU rows
  {
    if(!m_ThPI.isActive()) //no active thread pool - encode entire picture at once
    {
      xEncodeHuffSlc(CoeffsQuantScanV, 0, 0, m_NumMCUsInArea - 1);
    }
    else //encode picture in MCU 
    {
      for(int32 MCU_RowIdx = 0; MCU_RowIdx < m_NumMCUsInHeight; MCU_RowIdx++) //loop over MCUs rows
      {
        const int32 MCU_IdxFirst = MCU_RowIdx   * m_NumMCUsInWidth;
        const int32 MCU_IdxLast  = MCU_IdxFirst + m_NumMCUsInWidth - 1;        
        m_ThPI.storeTask([this, &CoeffsQuantScanV, MCU_RowIdx, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/) { xEncodeHuffChk(CoeffsQuantScanV, MCU_RowIdx, MCU_IdxFirst, MCU_IdxLast); });
      }
    }
  }
  else //divide picture into independent slices
  {
    for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++) //loop over slices
    {
      const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
      const int32 MCU_IdxLast  = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_RestartInterval) - 1;
      m_ThPI.storeTask([this, &CoeffsQuantScanV, SliceIdx, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/) { xEncodeHuffSlc(CoeffsQuantScanV, SliceIdx, MCU_IdxFirst, MCU_IdxLast); });
    }
  }
  m_ThPI.executeStoredTasks();
}
void xAdvancedEncoder::xEncodeHuffSlc(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  m_EncBuffers     [SliceIdx].reset();
  m_EntropyHuffEncs[SliceIdx].StartSlice(&m_EncBuffers[SliceIdx]);
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++) { xEncodeHuffMCU(CoeffsQuantScanV, SliceIdx, MCU_Idx); }
  m_EntropyHuffEncs[SliceIdx].FinishSlice();
  m_SliceBuffers   [SliceIdx].reset();
  xJFIF::AddStuffing(&m_SliceBuffers[SliceIdx], &m_EncBuffers[SliceIdx]);
}
void xAdvancedEncoder::xEncodeHuffChk(const int16* CoeffsQuantScanV[], int32 ChunkIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  xEntropyHuffCommon::tLDCs LastDCs = { 0,0,0,0 };
  if(MCU_IdxFirst != 0)
  {
    for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
    {
      const int32 BlockIdx    = MCU_IdxFirst * m_SampFactorVer[CmpIdx] * m_SampFactorHor[CmpIdx];
      const int32 CoeffOffset = BlockIdx << c_L2BA;
      const int16 LastDC      = CoeffsQuantScanV[CmpIdx][CoeffOffset - c_BA];
      LastDCs[CmpIdx] = LastDC;
    }
  }

  m_EncBuffers     [ChunkIdx].reset();
  m_EntropyHuffEncs[ChunkIdx].StartChunk(&m_EncBuffers[ChunkIdx], LastDCs);
  for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++) { xEncodeHuffMCU(CoeffsQuantScanV, ChunkIdx, MCU_Idx); }
  m_NumAlignmentBits[ChunkIdx] = m_EntropyHuffEncs[ChunkIdx].FinishChunk();
}
void xAdvancedEncoder::xEncodeHuffMCU(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_Idx)
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
        m_EntropyHuffEncs[SliceIdx].EncodeBlock(CoeffsQuantScanV[CmpIdx] + CoeffScanOffset, eCmp(CmpIdx), EntropyIdDC, EntropyIdAC);
        BlockIdx++;
      }
    }
  }
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG