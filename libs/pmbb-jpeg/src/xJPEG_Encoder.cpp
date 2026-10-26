/*
    SPDX-FileCopyrightText: 2020-2026 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Encoder.h"
#include "xJPEG_Transform.h"
#include "xJPEG_TransformConstants.h"
#include "xJPEG_Scan.h"
#include "xJPEG_EntropyHuffman.h"
#include "xMemory.h"
#include "xPixelOps.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

void xAdvancedEncoder::create(int32V2 PictureSize, eCrF ChromaFormat, eEntrTp EntropyType, xThreadPool* ThreadPool)
{
  m_EntropyType = EntropyType;

  initCodecCommon(PictureSize, ChromaFormat);

  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32 Area = m_MCUsMulWidth[CmpIdx] * m_MCUsMulHeight[CmpIdx];
    m_CmpCoeffsTransOrg    [CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16));
    m_CmpCoeffsTransRec    [CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16)); //TODO only if ????
    m_CmpCoeffsQuantScan   [CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16));
    m_CmpCoeffsQuantScanAux[CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16)); //TODO only if RDOQ
    m_CmpCoeffsQuantScanOpt[CmpIdx] = (int16*)xMemory::xAlignedMallocPageAuto(Area * sizeof(int16)); //TODO only if RDOQ
  }

  m_PicRec.create(PictureSize, 8, ChromaFormat, 16);

  if(ThreadPool != nullptr)
  {
    m_ThPI.init(ThreadPool, m_PictureSize.getY(), m_PictureSize.getY());
  }
}
void xAdvancedEncoder::destroy()
{
  m_ThPI.uninit();

  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    if(m_CmpCoeffsTransOrg    [CmpIdx] != nullptr) {xMemory::xAlignedFreeNull(m_CmpCoeffsTransOrg    [CmpIdx]); }
    if(m_CmpCoeffsTransRec    [CmpIdx] != nullptr) {xMemory::xAlignedFreeNull(m_CmpCoeffsTransRec    [CmpIdx]); }
    if(m_CmpCoeffsQuantScan   [CmpIdx] != nullptr) {xMemory::xAlignedFreeNull(m_CmpCoeffsQuantScan   [CmpIdx]); }
    if(m_CmpCoeffsQuantScanAux[CmpIdx] != nullptr) {xMemory::xAlignedFreeNull(m_CmpCoeffsQuantScanAux[CmpIdx]); }
    if(m_CmpCoeffsQuantScanOpt[CmpIdx] != nullptr) {xMemory::xAlignedFreeNull(m_CmpCoeffsQuantScanOpt[CmpIdx]); }
  }

  //m_EntropyBuffer.destroy();
  m_PicRec.destroy();
}
void xAdvancedEncoder::setMarkerEmit(bool EmitAPP0, bool EmitQuantTabs, bool EmitEntropyTabs)
{
  m_EmitAPP0        = EmitAPP0       ;
  m_EmitQuantTabs   = EmitQuantTabs  ;
  m_EmitEntropyTabs = EmitEntropyTabs;
}
void xAdvancedEncoder::setOptQuant(bool OptQuantCoeffs, eQntOpt OptQuantAlgorithm, bool ProcessZeroCoeffs, int32 NumOptBestCands, eLmbd LambdaEstMode)
{ 
  m_OptQuantCoeffs    = OptQuantCoeffs   ;
  m_OptQuantAlgorithm = OptQuantAlgorithm;
  m_ProcessZeroCoeffs = ProcessZeroCoeffs;
  m_NumOptBestCands   = NumOptBestCands  ;
  m_LambdaEstMode     = LambdaEstMode    ;
}
void xAdvancedEncoder::setOptEntropy(bool OptEntropyTables)
{
  m_OptEntropyTables = OptEntropyTables;
}
void xAdvancedEncoder::setOptPass(int32 NumBlockOptPasses, int32 NumPicOptPasses)
{
  m_NumOptPassesBlock = NumBlockOptPasses;
  m_NumOptPassesPic   = NumPicOptPasses  ;
}
void xAdvancedEncoder::setOptClcMode(eCalkMd BlkOptDistCalcMode, eCalkMd BlkOptBitsCalcMode)
{
  m_BlkOptDistCalcMode = BlkOptDistCalcMode;
  m_BlkOptBitsCalcMode = BlkOptBitsCalcMode;
}
void xAdvancedEncoder::initBaseMarkers()
{
  int32 TypeSOF = NOT_VALID;
  if(m_EntropyType == eEntrTp::Huffman   ) { TypeSOF = 0; }
  if(m_EntropyType == eEntrTp::Arithmetic) { TypeSOF = 9; }

  //init markers
  m_APP0.InitDefault();
  m_SOF .Init(TypeSOF, m_PictureHeight, m_PictureWidth, 8, m_ChromaFormat, 2);
  if(m_EntropyType == eEntrTp::Arithmetic && m_OptEntropyTables) { m_SOS.InitExtended(m_SOF.getNumComponents(), { 0, 1, 2, NOT_VALID }); }
  else { m_SOS.InitBaseline(m_SOF.getNumComponents(), 0, 1); }
}
void xAdvancedEncoder::initQuant(int32 Quality, eQTLa QuantTabLayout)
{
  m_Quality        = Quality;
  m_QuantTabLayout = QuantTabLayout;

  //init markers
  m_QTs.resize(2);
  m_QTs[0].Init(0, eCmp::LM, Quality, QuantTabLayout);
  m_QTs[1].Init(1, eCmp::CB, Quality, QuantTabLayout); //any chroma so use CB

  if(m_VerboseLevel >= 6)
  {
    std::string Dump = fmt::format("QuantizationTablesMain Num={}\n", m_QTs.size());
    for(int32 i = 0; i < (int32)m_QTs.size(); i++) { Dump += fmt::format("  QuantTab_{:d}\n", i) + m_QTs[i].Format("    "); }
    fmt::print("{}", Dump);
  }

  //init toolbox
  m_QuantMain.Init(m_QTs);
  if(m_OptQuantCoeffs && (m_LambdaEstMode == eLmbd::Exhaustive3 || m_LambdaEstMode == eLmbd::Exhaustive5))
  {
    std::vector<int32> DeltaQs;
    if(m_LambdaEstMode == eLmbd::Exhaustive3) { DeltaQs = { -2, 2 }; }
    if(m_LambdaEstMode == eLmbd::Exhaustive5) { DeltaQs = { -2, -1, 1, 2 }; }

    for(const int32 DeltaQ : DeltaQs)
    {
      if(m_QuantAuxs.find(DeltaQ) == m_QuantAuxs.end()) { m_QuantAuxs.emplace(DeltaQ, xQuantizerSet{}); }
      m_QuantAuxs[DeltaQ].Init(0, eCmp::LM, Quality + DeltaQ, QuantTabLayout);
      m_QuantAuxs[DeltaQ].Init(1, eCmp::CB, Quality + DeltaQ, QuantTabLayout); //any chroma so use CB
    }
  }

  if(m_VerboseLevel >= 6)
  {
    std::string Dump = fmt::format("QuantizerTablesMain\n");
    for(int32 i = 0; i < (int32)m_QTs.size(); i++)
    {
      Dump += fmt::format("  Table_{}\n", i) + m_QuantMain.getQuantizer(i).FormatCoeffs("    ");
    }
    fmt::print("{}", Dump);
  }
}
void xAdvancedEncoder::initEntropy(int32 RestartInterval)
{
  m_RestartInterval = RestartInterval;  

  if(m_EntropyType == eEntrTp::Huffman   ) { m_HTs = xJFIF::xHuffTable::createDefaultHuffTables(); }
  if(m_EntropyType == eEntrTp::Arithmetic) { m_ACs = xJFIF::xArithCond::createDefaultArithCond (!m_OptEntropyTables); }

  m_NumOfSlices    = RestartInterval > 0 ? (int32)std::ceil((flt64)m_NumMCUsInArea / (flt64)RestartInterval) : 1;
  m_NumMCUsInSlice = RestartInterval != 0 ? RestartInterval : m_NumMCUsInArea;
  int32 NumBlocksInMCU       = std::accumulate(m_NumBlocksInMCU.cbegin(), m_NumBlocksInMCU.cend(), 0);
  int32 MaxEncodedSliceSize  = m_NumMCUsInSlice * NumBlocksInMCU * c_BA * 2;
  int32 MaxEncodedMCURowSize = m_NumMCUsInWidth * NumBlocksInMCU * c_BA * 2;
  
  bool  EncodeMCU_Rows = RestartInterval == 0 && m_EntropyType == eEntrTp::Huffman && m_ThPI.isActive();
  int32 NumEncoders    = EncodeMCU_Rows ? m_NumMCUsInHeight    : m_NumOfSlices      ;
  int32 EncBufferSize  = EncodeMCU_Rows ? MaxEncodedMCURowSize : MaxEncodedSliceSize;

  //buffers
  m_EncBuffers  .resize(NumEncoders);
  m_SliceBuffers.resize(NumEncoders);
  for(int32 i = 0; i < (int32)m_EncBuffers  .size(); i++) { m_EncBuffers  [i].resize(EncBufferSize); }
  for(int32 i = 0; i < (int32)m_SliceBuffers.size(); i++) { m_SliceBuffers[i].resize(EncBufferSize); }

  if(EncodeMCU_Rows)
  {
    m_NumAlignmentBits.resize(m_NumMCUsInHeight, 0);
    const int32 CombineBufferSize = m_NumMCUsInArea * NumBlocksInMCU * c_BA * 2;
    m_CombineBuffer.create(CombineBufferSize);
  }

  if(m_EntropyType == eEntrTp::Huffman)
  {
    m_EntropyHuffEst .Init(m_HTs);
    m_EncoderHuffBank.Init(m_HTs);
    m_EntropyHuffEncs = std::vector<xEntropyHuffEncoder>(NumEncoders);
    for(int32 i = 0; i < (int32)m_EntropyHuffEncs.size(); i++) { m_EntropyHuffEncs[i].SetEncoders(m_EncoderHuffBank); }
  }
  if(m_EntropyType == eEntrTp::Arithmetic)
  {
    m_EntropyAritEsts.resize(NumEncoders);
    m_EntropyAritEncs.resize(NumEncoders);
    for(int32 i = 0; i < (int32)m_EntropyAritEsts.size(); i++) { m_EntropyAritEsts[i].Init(m_ACs); }
    for(int32 i = 0; i < (int32)m_EntropyAritEncs.size(); i++) { m_EntropyAritEncs[i].Init(m_ACs); }
  }
  if(m_OptEntropyTables && m_EntropyType == eEntrTp::Huffman)
  {
    int32 NumHuffCounters = m_ThPI.isActive() ? m_ThPI.getNumThreads() : 1;
    m_EntropyHuffCnts.resize(NumHuffCounters);
  }
}
void xAdvancedEncoder::encode(const xPicYUV* InputPicture, xByteBuffer* OutputBuffer)
{
  xEncodePicture(OutputBuffer, InputPicture);
}
std::string xAdvancedEncoder::formatAndResetStats(const std::string Prefix, flt64 TicksPerMicroSec)
{
  if(!m_GatherTimeStats      ) { return "Time stats gathering is disabled!"; }
  if(m_TotalPictureIters == 0) { return "No time stats gathered!"; }

  flt64 InvDenom = TicksPerMicroSec == 0.0 ? (flt64)1.0 / ((flt64)m_TotalPictureIters) : (flt64)1.0 / ((flt64)m_TotalPictureIters * TicksPerMicroSec);

  std::string TimeStats; TimeStats.reserve(xMemory::getBestEffortSizePageBase());
  TimeStats += Prefix + fmt::format("Processing time {}\n", TicksPerMicroSec == 0.0 ? "[ticks]" : "[us]");

  TimeStats += Prefix + "  " + fmt::format("PicIters   = {}\n"    , m_TotalPictureIters);
  TimeStats += Prefix + "  " + fmt::format("Pic        = {:.2f}\n", m_Ticks___Picture * InvDenom);
  TimeStats += Prefix + "  " + fmt::format("Transform  = {:.2f}\n", m_Ticks_Transform * InvDenom);
  TimeStats += Prefix + "  " + fmt::format("QuantScan  = {:.2f}\n", m_Ticks_QuantScan * InvDenom);
  TimeStats += Prefix + "  " + fmt::format("LambdaEst  = {:.2f}\n", m_Ticks_LambdaEst * InvDenom);
  TimeStats += Prefix + "  " + fmt::format("OptQuant   = {:.2f}\n", m_Ticks__OptQuant * InvDenom);
  TimeStats += Prefix + "  " + fmt::format("OptEntropy = {:.2f}\n", m_TicksOptEntropy * InvDenom);
  TimeStats += Prefix + "  " + fmt::format("EntropyEnc = {:.2f}\n", m_TicksEntropyEnc * InvDenom);
  TimeStats += Prefix + "  " + fmt::format("Tables     = {:.2f}\n", m_Ticks____Tables * InvDenom);
  TimeStats += Prefix + "  " + fmt::format("WriteOut   = {:.2f}\n", m_Ticks__WriteOut * InvDenom);
  
  m_Ticks_Transform = 0;
  m_Ticks_QuantScan = 0;
  m_Ticks_LambdaEst = 0;
  m_Ticks__OptQuant = 0;
  m_TicksOptEntropy = 0;
  m_TicksEntropyEnc = 0;
  m_Ticks____Tables = 0;
  m_Ticks__WriteOut = 0;

  m_TotalPictureIters = 0;
  m_TotalSliceIters   = 0;

  return TimeStats;
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

void xAdvancedEncoder::xEncodePicture(xByteBuffer* OutputBuffer, const xPicYUV* Picture)
{
  const int16* ConstCmpCoeffsTransOrg    [] = { m_CmpCoeffsTransOrg    [0], m_CmpCoeffsTransOrg    [1], m_CmpCoeffsTransOrg    [2], m_CmpCoeffsTransOrg    [3]};
  const int16* ConstCmpCoeffsQuantScan   [] = { m_CmpCoeffsQuantScan   [0], m_CmpCoeffsQuantScan   [1], m_CmpCoeffsQuantScan   [2], m_CmpCoeffsQuantScan   [3]};
  const int16* ConstCmpCoeffsQuantScanOpt[] = { m_CmpCoeffsQuantScanOpt[0], m_CmpCoeffsQuantScanOpt[1], m_CmpCoeffsQuantScanOpt[2], m_CmpCoeffsQuantScanOpt[3]};

  if(m_OptEntropyTables)
  {
    if(m_EntropyType == eEntrTp::Huffman   ) { m_HTs = xJFIF::xHuffTable::createDefaultHuffTables(     ); } //make sure that new pics starts with m_HTs set to DefaultHuffTables
    if(m_EntropyType == eEntrTp::Arithmetic) //make sure that new pics starts with m_ACs set to DefaultArithCond 
    {
      m_ACs = xJFIF::xArithCond::createDefaultArithCond(false);
      for(int32 i = 0; i < (int32)m_EntropyAritEsts.size(); i++) { m_EntropyAritEsts[i].Init(m_ACs); }
      for(int32 i = 0; i < (int32)m_EntropyAritEncs.size(); i++) { m_EntropyAritEncs[i].Init(m_ACs); }
    } 
  }

  uint64 TP0 = m_GatherTimeStats ? xTSC() : 0;

  xFwdTransformPic(m_CmpCoeffsTransOrg, Picture);

  uint64 TP1 = m_GatherTimeStats ? xTSC() : 0;

  xFwdQuantScanPic(m_CmpCoeffsQuantScan, ConstCmpCoeffsTransOrg, m_QuantMain);

  uint64 TP2 = m_GatherTimeStats ? xTSC() : 0;

  if(m_OptQuantCoeffs) { xEstimateLambda(Picture); }

  uint64 TP3 = m_GatherTimeStats ? xTSC() : 0;

  for(int32 n = 0; n < m_NumOptPassesPic; n++)
  {
    uint64 TPo0 = m_GatherTimeStats ? xTSC() : 0;

    if(m_OptQuantCoeffs) { xOptQuantPic(m_CmpCoeffsQuantScanOpt, ConstCmpCoeffsQuantScan, ConstCmpCoeffsTransOrg, Picture); }

    uint64 TPo1 = m_GatherTimeStats ? xTSC() : 0;

    if(m_OptEntropyTables) { xOptEntropyPic(m_OptQuantCoeffs ? ConstCmpCoeffsQuantScanOpt : ConstCmpCoeffsQuantScan); }

    if(m_GatherTimeStats) { m_Ticks__OptQuant += TPo1 - TPo0; m_TicksOptEntropy += xTSC() - TPo1; }
  }

  uint64 TP4 = m_GatherTimeStats ? xTSC() : 0;

  xEncodePic(m_OptQuantCoeffs ? ConstCmpCoeffsQuantScanOpt : ConstCmpCoeffsQuantScan);

  uint64 TP5 = m_GatherTimeStats ? xTSC() : 0;

  xJFIF::WriteSOI(OutputBuffer);
  if(m_EmitAPP0       ) { xJFIF::WriteAPP0(OutputBuffer, m_APP0); }
  if(m_EmitQuantTabs  ) { xJFIF::WriteDQT (OutputBuffer, m_QTs); }
  if(m_RestartInterval) { xJFIF::WriteDRI (OutputBuffer, m_RestartInterval); }
  xJFIF::WriteSOF(OutputBuffer, m_SOF);
  if(m_EmitEntropyTabs && m_EntropyType == eEntrTp::Huffman   ) { xJFIF::WriteDHT(OutputBuffer, m_HTs); }
  if(m_EmitEntropyTabs && m_EntropyType == eEntrTp::Arithmetic) { xJFIF::WriteDAC(OutputBuffer, m_ACs); }

  uint64 TP6 = m_GatherTimeStats ? xTSC() : 0;

  xJFIF::WriteSOS(OutputBuffer, m_SOS);
  xWritePic(OutputBuffer);
  xJFIF::WriteEOI(OutputBuffer);

  uint64 TP7 = m_GatherTimeStats ? xTSC() : 0;
  
  m_TotalPictureIters += 1;
  if(m_GatherTimeStats)
  {
    m_Ticks___Picture += TP6 - TP0;
    m_Ticks_Transform += TP1 - TP0;
    m_Ticks_QuantScan += TP2 - TP1;
    m_Ticks_LambdaEst += TP3 - TP2;
    m_TicksEntropyEnc += TP5 - TP4;
    m_Ticks____Tables += TP6 - TP5;
    m_Ticks__WriteOut += TP7 - TP6;
  }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Huffman tables & Arithmetic conditioning optimization
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xOptEntropyPic(const int16* CoeffsQuantScanV[])
{
  if     (m_EntropyType == eEntrTp::Huffman   ) { xOptHuffPic(CoeffsQuantScanV); }
  else if(m_EntropyType == eEntrTp::Arithmetic) { xOptAritPic(CoeffsQuantScanV); }
  else                                          { assert(0); }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Huffman & arithmetic encode
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xEncodePic(const int16* CoeffsQuantScanV[])
{
  if(m_EntropyType == eEntrTp::Huffman   ) { xEncodeHuffPic(CoeffsQuantScanV); }
  if(m_EntropyType == eEntrTp::Arithmetic) { xEncodeAritPic(CoeffsQuantScanV); }
}

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Writeout
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void xAdvancedEncoder::xWritePic(xByteBuffer* OutputBuffer)
{
  if(m_RestartInterval == 0)
  {
    if(!m_ThPI.isActive() || m_EntropyType == eEntrTp::Arithmetic)
    {
      OutputBuffer->transfer(&m_SliceBuffers[0]);
    }
    else
    {
      m_CombineBuffer.reset();
      xBitstreamWriter Bitstream;
      Bitstream.bindByteBuffer(&m_CombineBuffer);
      Bitstream.init();
      for(int32 MCU_RowIdx = 0; MCU_RowIdx < m_NumMCUsInHeight; MCU_RowIdx++)
      {
        Bitstream.writeBuffer(&m_EncBuffers[MCU_RowIdx], m_NumAlignmentBits[MCU_RowIdx]);
      }
      Bitstream.writeAlign(1);
      Bitstream.uninit();
      Bitstream.unbindByteBuffer();
      xJFIF::AddStuffing(OutputBuffer, &m_CombineBuffer);
    }
  }
  else
  {
    for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++)
    {
      const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
      const int32 MCU_IdxLast = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_RestartInterval) - 1;
      //copy to output and add stuffing
      OutputBuffer->transfer(&m_SliceBuffers[SliceIdx]);
      if(m_RestartInterval >= 0)
      {
        if(MCU_IdxLast != m_NumMCUsInArea - 1)
        {
          xJFIF::WriteRST(OutputBuffer, (uint8)((uint32)SliceIdx & (uint32)0x07));
        }
      }
    }
  }
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG