/*
    SPDX-FileCopyrightText: 2020-2026 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-FileCopyrightText: 2026 Krzysztof Klimaszewski <krzysztof.klimaszewski@put.poznan.pl>
    SPDX-FileCopyrightText: 2026 Kacper Winiecki <kacper.winiecki@student.put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Encoder.h"
#include "xJPEG_OptUtils.h"
#include "xDistortion.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// xAdvancedEncoder - Lambda estimation
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
template<eEntrTp ET> int64V4 xAdvancedEncoder::xtCalcBitsPic(const int16* CoeffsQuantScanV[])
{
  std::vector<int64V4> EstNumBits;
  if(m_RestartInterval == 0) 
  {
    if constexpr(ET == eEntrTp::Huffman) ////no division - however, process picture in MCU rows
    {
      EstNumBits.resize(m_NumMCUsInHeight);
      for(int32 MCU_RowIdx = 0; MCU_RowIdx < m_NumMCUsInHeight; MCU_RowIdx++) //loop over MCUs rows
      {
        const int32 MCU_IdxFirst = MCU_RowIdx   * m_NumMCUsInWidth    ;
        const int32 MCU_IdxLast  = MCU_IdxFirst + m_NumMCUsInWidth - 1;
        m_ThPI.storeTask([this, &EstNumBits, MCU_RowIdx, &CoeffsQuantScanV, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/)
          { EstNumBits[MCU_RowIdx] = xtCalcBitsSlc<ET>(CoeffsQuantScanV, MCU_RowIdx, MCU_IdxFirst, MCU_IdxLast); });
      }
    }
    else //no division - serial processing
    {
      int64V4 TotalEstNumBits = xtCalcBitsSlc<ET>(CoeffsQuantScanV, 0, 0, m_NumMCUsInArea - 1);
      return TotalEstNumBits;
    }
  }
  else
  {
    EstNumBits.resize(m_NumOfSlices);
    for(int32 SliceIdx = 0; SliceIdx < m_NumOfSlices; SliceIdx++) //loop over slices
    {
      const int32 MCU_IdxFirst = SliceIdx * m_RestartInterval;
      const int32 MCU_IdxLast  = xMin(m_NumMCUsInArea, MCU_IdxFirst + m_NumMCUsInSlice) - 1;
      m_ThPI.storeTask([this, &EstNumBits, SliceIdx, &CoeffsQuantScanV, MCU_IdxFirst, MCU_IdxLast](int32 /*ThIdx*/)
        { EstNumBits[SliceIdx] = xtCalcBitsSlc<ET>(CoeffsQuantScanV, SliceIdx, MCU_IdxFirst, MCU_IdxLast); });
    }
  }

  m_ThPI.executeStoredTasks();
  int64V4 TotalEstNumBits = std::accumulate(EstNumBits.cbegin(), EstNumBits.cend(), xMakeVec4<int64>(0));
  return TotalEstNumBits;
}
template<eEntrTp ET> int64V4 xAdvancedEncoder::xtCalcBitsSlc(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast)
{
  int64V4 EstNumBits = xMakeVec4<int64>(0);
  if constexpr(ET == eEntrTp::Huffman)
  {
    for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++) //loop over MCUs
    {
      EstNumBits += xtCalcBitsMCU<ET>(CoeffsQuantScanV, SliceIdx, MCU_Idx);
    }
  }
  else
  {
    m_EntropyAritEsts[SliceIdx].StartSlice();
    for(int32 MCU_Idx = MCU_IdxFirst; MCU_Idx <= MCU_IdxLast; MCU_Idx++) //loop over MCUs
    {
      xtCalcBitsMCU<ET>(CoeffsQuantScanV, SliceIdx, MCU_Idx);
    }
    m_EntropyAritEsts[SliceIdx].FinishSlice();
    EstNumBits = m_EntropyAritEsts[SliceIdx].getNumBits();
  }
  return EstNumBits;
}
template<eEntrTp ET> int64V4 xAdvancedEncoder::xtCalcBitsMCU(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_Idx)
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
        if constexpr(ET == eEntrTp::Huffman)
        {
          const bool  FirstInSlc = MCU_FirstInSlice && V == 0 && H == 0;
          const int32 LastDC     = FirstInSlc ? 0 : CoeffsQuantScanV[CmpIdx][CoeffOffset - c_BA];
          EstNumBits[CmpIdx] += m_EntropyHuffEst.EstimateBlock(CoeffsQuantScanV[CmpIdx] + CoeffOffset, LastDC, EntropyIdDC, EntropyIdAC);
        }
        else
        {
          m_EntropyAritEsts[SliceIdx].EncodeBlock(CoeffsQuantScanV[CmpIdx] + CoeffOffset, (eCmp)CmpIdx, EntropyIdDC, EntropyIdAC);
        }
        BlockIdx++;
      } //H
    } //V
  } //CmpIdx

  return EstNumBits;
}
int64V4 xAdvancedEncoder::xCalcBitsPic(const int16* CoeffsQuantScanV[])
{
  if     (m_EntropyType == eEntrTp::Huffman   ) { return xCalcBitsHuffPic(CoeffsQuantScanV); }
  else if(m_EntropyType == eEntrTp::Arithmetic) { return xCalcBitsAritPic(CoeffsQuantScanV); }
  else                                          { return xMakeVec4<int64>(NOT_VALID); }

  //if     (m_EntropyType == eEntrTp::Huffman   ) { return xtCalcBitsPic<eEntrTp::Huffman   >(CoeffsQuantScanV); }
  //else if(m_EntropyType == eEntrTp::Arithmetic) { return xtCalcBitsPic<eEntrTp::Arithmetic>(CoeffsQuantScanV); }
  //else                                          { return xMakeVec4<int64>(NOT_VALID); }
}
int64V4 xAdvancedEncoder::xCalcDistPicSSD(const xPicYUV* Tst, const xPicYUV* Ref)
{
  int64V4 SSDs = xMakeVec4<int64>(0);
  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {    
    m_ThPI.storeTask([&SSDs, &Tst, &Ref, CmpIdx] (int32 /*ThIdx*/) { eCmp c = (eCmp)CmpIdx; SSDs[CmpIdx] = xDistortion::CalcSSD(Tst->getAddr(c), Ref->getAddr(c), Tst->getStride(c), Ref->getStride(c), Tst->getWidth(c), Tst->getHeight(c), Ref->getBitDepth()); });
  }
  m_ThPI.executeStoredTasks();

  return SSDs;
}
int64V4 xAdvancedEncoder::xEstDistPicSSD(const int16* CoeffsTransRecV[], const int16* CoeffsTransOrgV[])
{
  auto EstDistCmp = [](const int16* CoeffsTransRec, const int16* CoeffsTransOrg, int32 NumBlocks)
    {
      uint64 EstSSD = 0;
      for(int32 BlockIdx = 0; BlockIdx < NumBlocks; BlockIdx++)
      {
        const int32 BlockOffset = BlockIdx << c_L2BA;
        EstSSD += xOptUtils::approxSSDfromCoeffs(CoeffsTransRec + BlockOffset, CoeffsTransOrg + BlockOffset);
      }
      return EstSSD;
    };

  int64V4 EstSSDs = xMakeVec4<int64>(0);

  for(int32 CmpIdx = 0; CmpIdx < m_NumOfComponents; CmpIdx++)
  {
    const int32 NumBlocksInArea = m_NumBlocksInArea[CmpIdx];

    const int16* CoeffsTransOrg = CoeffsTransOrgV[CmpIdx];
    const int16* CoeffsTransRec = CoeffsTransRecV[CmpIdx];

    m_ThPI.storeTask([&EstSSDs, &EstDistCmp, CmpIdx, CoeffsTransOrg, CoeffsTransRec, NumBlocksInArea](int32 /*ThIdx*/)
      { EstSSDs[CmpIdx] = EstDistCmp(CoeffsTransRec, CoeffsTransOrg, NumBlocksInArea); });

  }
  m_ThPI.executeStoredTasks();
  return EstSSDs;
}
int64V4 xAdvancedEncoder::xCalcDistPic(const int16* CoeffsQuantScanV[], const xQuantizerSet& Quant, const xPicYUV* PictureRef)
{
  const int16* ConstCmpCoeffsTransOrg[] = { m_CmpCoeffsTransOrg[0], m_CmpCoeffsTransOrg[1], m_CmpCoeffsTransOrg[2], m_CmpCoeffsTransOrg[3] };
  const int16* ConstCmpCoeffsTransRec[] = { m_CmpCoeffsTransRec[0], m_CmpCoeffsTransRec[1], m_CmpCoeffsTransRec[2], m_CmpCoeffsTransRec[3] };

  xInvScanQuantPic(m_CmpCoeffsTransRec, CoeffsQuantScanV, Quant);

  int64V4 SSDs = xMakeVec4<int64>(0);

  if(m_BlkOptDistCalcMode == eCalkMd::Exact)
  {
    xInvTransformPic(&m_PicRec, ConstCmpCoeffsTransRec);
    SSDs = xCalcDistPicSSD(PictureRef, &m_PicRec);
  }
  else //m_BlkOptDistCalcMode == eCalkMd::Approx
  {
    SSDs = xEstDistPicSSD(ConstCmpCoeffsTransRec, ConstCmpCoeffsTransOrg);
  }
  return SSDs;
}
void xAdvancedEncoder::xEstimateLambda(const xPicYUV* Picture)
{
  static constexpr flt64 NaN = std::numeric_limits<flt64>::quiet_NaN();

  if(m_LambdaEstMode == eLmbd::ApproxFast)
  {    
    const int32 Q   = xClip(m_Quality, 1, 100);
    const flt64 LnQ = std::log((flt64)Q);

    flt64 a = NaN, b = NaN, c = NaN, d = NaN;
    flt64 LambdaL = NaN, LambdaC = NaN;

    if(m_EntropyType == eEntrTp::Huffman)
    {
      switch(m_QuantTabLayout)
      {
      case eQTLa::Default : a = -0.6203130; b =  6.35181; c = -23.21140; d = 33.71200; break;
      case eQTLa::Flat    : a =  0.4740840; b = -5.31506; c =  17.14060; d = -8.78472; break;
      case eQTLa::SemiFlat: a =  0.0740126; b = -1.03562; c =   2.31849; d =  6.84053; break;
      default             : assert(0); break;
      }    
      LambdaL = std::exp(a * xPow3(LnQ) + b * xPow2(LnQ) + c * LnQ + d);

      switch(m_QuantTabLayout)
      {
      case eQTLa::Default : a = -0.497367 ; b =  5.81172; c = -24.5218 ; d =  40.4226 ; break; 
      case eQTLa::Flat    : a =  0.487363 ; b = -6.41458; c =  24.3768 ; d = -18.950  ; break;
      case eQTLa::SemiFlat: a =  0.0441953; b = -1.53874; c =   6.62694; d =   1.13215; break;
      default             : assert(0); break;
      }
      LambdaC = std::exp(a * xPow3(LnQ) + b * xPow2(LnQ) + c * LnQ + d);
    }
    else if(m_EntropyType == eEntrTp::Arithmetic)
    {
      switch(m_QuantTabLayout)
      {
      case eQTLa::Default : a = -0.415579 ; b =  4.231790; c = -16.0139000; d = 25.73670; break;
      case eQTLa::Flat    : a =  0.177291 ; b = -2.317670; c =   7.3645200; d =  1.38623; break;
      case eQTLa::SemiFlat: a = -0.0239345; b = -0.161221; c =  -0.0991587; d =  8.80884; break;
      default             : assert(0); break;
      }    
      LambdaL = std::exp(a * xPow3(LnQ) + b * xPow2(LnQ) + c * LnQ + d);

      switch(m_QuantTabLayout)
      {
      case eQTLa::Default : a = -0.346198; b =  3.47941; c = -13.3413; d =  23.2924; break; 
      case eQTLa::Flat    : a =  0.870995; b = -9.62951; c =  32.6524; d = -26.1471; break;
      case eQTLa::SemiFlat: a =  0.482103; b = -5.60066; c =  19.1170; d = -12.5722; break;
      default             : assert(0); break;
      }
      LambdaC = std::exp(a * xPow3(LnQ) + b * xPow2(LnQ) + c * LnQ + d);
    }
    
    m_Lambda = { LambdaL, LambdaC, LambdaC, NaN };
  }
  else if(m_LambdaEstMode == eLmbd::ApproxExact)
  {
    const int16* ConstCmpCoeffsQuantScan[] = { m_CmpCoeffsQuantScan[0], m_CmpCoeffsQuantScan[1], m_CmpCoeffsQuantScan[2], m_CmpCoeffsQuantScan[3] };

    //current point
    int64V4 EstNumBits = xCalcBitsPic(ConstCmpCoeffsQuantScan);
    int64V4 Distortion = xCalcDistPic(ConstCmpCoeffsQuantScan, m_QuantMain, Picture);

    int32 AreaL         = m_CmpWidth[0] * m_CmpHeight[0];
    int32 AreaC         = m_CmpWidth[1] * m_CmpHeight[1] + m_CmpWidth[2] * m_CmpHeight[2];
    flt64 BitsPerPixelL = flt64(EstNumBits[0]                ) / AreaL;
    flt64 BitsPerPixelC = flt64(EstNumBits[1] + EstNumBits[2]) / AreaC;
    flt64 MSE_L         = flt64(Distortion[0]                ) / AreaL;
    flt64 MSE_C         = flt64(Distortion[1] + Distortion[2]) / AreaC;

    const int32 Q    = xClip(m_Quality, 1, 100);
    const flt64 LnQ  = std::log((flt64)Q     );
    const flt64 LnBL = std::log(BitsPerPixelL);
    const flt64 LnBC = std::log(BitsPerPixelC);
    const flt64 LnML = std::log(MSE_L        );
    const flt64 LnMC = std::log(MSE_C        );

    flt64 a = NaN, b = NaN, c = NaN, d = NaN, e = NaN, f = NaN, g = NaN;
    flt64 LambdaL = NaN, LambdaC = NaN;

    if(m_EntropyType == eEntrTp::Huffman)
    {
      switch(m_QuantTabLayout)
      {
      case eQTLa::Default : a= 2.57992; b=-0.35913; c=-0.63476; d=0.77027; e=-0.03329; f=-0.02950; g=0.19130; break;
      case eQTLa::Flat    : a=-1.63326; b= 0.66666; c=-2.16234; d=0.88686; e= 0.28395; f=-0.04947; g=0.27516; break;
      case eQTLa::SemiFlat: a= 1.43200; b=-0.45698; c=-0.98724; d=0.90448; e= 0.01148; f= 0.04738; g=0.25582; break;
      default             : assert(0); break;
      }
      LambdaL = std::exp(a + b * LnQ + c * LnBL + d * LnML + e * (LnQ * LnBL) + f * (LnQ * LnQ) + g * (LnBL * LnBL));
    
      switch(m_QuantTabLayout)
      {
      case eQTLa::Default : a=  3.06943; b= 0.52480; c= -0.43136; d=0.64136; e=-0.07390; f=-0.25623; break;
      case eQTLa::Flat    : a=-95.03556; b=24.03112; c=-28.07712; d=0.95958; e= 5.84854; f=-0.72730; break;
      case eQTLa::SemiFlat: a=-32.71444; b=10.15678; c=-10.56762; d=0.66371; e= 2.05193; f=-0.67328; break;
      default             : assert(0); break;
      }
      LambdaC = std::exp(a + b * LnQ + c * LnBC + d * LnMC + e * (LnQ * LnBC) + f * (LnQ * LnQ));
    }
    else if(m_EntropyType == eEntrTp::Arithmetic)
    {

    }

    m_Lambda = { LambdaL, LambdaC, LambdaC, NaN };
  }
  else if(m_LambdaEstMode == eLmbd::Exhaustive3 || m_LambdaEstMode == eLmbd::Exhaustive5)
  {
    const int16* ConstCmpCoeffsTransOrg    [] = { m_CmpCoeffsTransOrg    [0], m_CmpCoeffsTransOrg    [1], m_CmpCoeffsTransOrg    [2], m_CmpCoeffsTransOrg    [3] };
    const int16* ConstCmpCoeffsQuantScan   [] = { m_CmpCoeffsQuantScan   [0], m_CmpCoeffsQuantScan   [1], m_CmpCoeffsQuantScan   [2], m_CmpCoeffsQuantScan   [3] };
    const int16* ConstCmpCoeffsQuantScanAux[] = { m_CmpCoeffsQuantScanAux[0], m_CmpCoeffsQuantScanAux[1], m_CmpCoeffsQuantScanAux[2], m_CmpCoeffsQuantScanAux[3] };

    //storage
    std::array<std::vector<int64>, 4> Dist;
    std::array<std::vector<int64>, 4> Bits;

    //current point
    int64V4 EstNumBitsMain = xCalcBitsPic(ConstCmpCoeffsQuantScan);
    int64V4 DistortionMain = xCalcDistPic(ConstCmpCoeffsQuantScan, m_QuantMain, Picture);
    for(int32 c = 0; c < 4; c++) { Dist[c].push_back(DistortionMain[c]); Bits[c].push_back(EstNumBitsMain[c]); }

    //auxliary points
    std::vector<int32> DeltaQs;
    if(m_LambdaEstMode == eLmbd::Exhaustive3) { DeltaQs = { -2, 2 }; }
    if(m_LambdaEstMode == eLmbd::Exhaustive5) { DeltaQs = { -2, -1, 1, 2 }; }

    for(const int32 DeltaQ : DeltaQs)
    {
      int32 Quality = m_Quality + DeltaQ;
      if(Quality < 1 || Quality > 100) { continue; }
      xFwdQuantScanPic(m_CmpCoeffsQuantScanAux, ConstCmpCoeffsTransOrg, m_QuantAuxs[DeltaQ]);
      int64V4 EstNumBitsAux = xCalcBitsPic(ConstCmpCoeffsQuantScanAux);
      int64V4 DistortionAux = xCalcDistPic(ConstCmpCoeffsQuantScanAux, m_QuantAuxs[DeltaQ], Picture);
      for(int32 c = 0; c < 4; c++) { Dist[c].push_back(DistortionAux[c]); Bits[c].push_back(EstNumBitsAux[c]); }
    }

    //lambda using linear regresion
    const int32 NumPoints = (int32)Dist[0].size();
    for(int32 c = 0; c < 4; c++)
    {
      flt64 SumD=0, SumB=0, SumBB=0, SumBD=0;
      for(int32 i = 0; i < NumPoints; i++)
      {
        flt64 D = (flt64)Dist[c][i];
        flt64 B = (flt64)Bits[c][i];
        SumD += D; SumB += B; SumBD += B*D; SumBB += xPow2(B);        
      }
      flt64 Slope = (NumPoints * SumBD - SumD * SumB) / (NumPoints * SumBB - SumB * SumB);
      m_Lambda[c] = -Slope;
    }
  }
  else { assert(0); }

  if(m_VerboseLevel >= 6)
  {
    fmt::print("Lambda = {} {} {} ", m_Lambda[0], m_Lambda[1], m_Lambda[2]);
  }
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG