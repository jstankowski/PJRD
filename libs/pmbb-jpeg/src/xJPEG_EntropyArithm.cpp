/*
    SPDX-FileCopyrightText: 2024-2025 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#include "xJPEG_EntropyArithm.h"
#include "xJPEG_EntropyUtils.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================
// xEntropyAritCommon
//=====================================================================================================================================================================================
#if PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
void xEntropyAritCommon::xFillDebugInfoDC(tArithModelsDC* M)
{
  if(M == nullptr) { return; }
  (*M)[0 ].setName("S0-Z ");
  (*M)[1 ].setName("SS-Z ");
  (*M)[2 ].setName("SP-Z ");
  (*M)[3 ].setName("SN-Z ");
  (*M)[4 ].setName("S0-SP");
  (*M)[5 ].setName("SS-SP");
  (*M)[6 ].setName("SP-SP");
  (*M)[7 ].setName("SN-SP");
  (*M)[8 ].setName("S0-SN");
  (*M)[9 ].setName("SS-SN");
  (*M)[10].setName("SP-SN");
  (*M)[11].setName("SN-SN");
  (*M)[12].setName("S0-LP");
  (*M)[13].setName("SS-LP");
  (*M)[14].setName("SP-LP");
  (*M)[15].setName("SN-LP");
  (*M)[16].setName("S0-LN");
  (*M)[17].setName("SS-LN");
  (*M)[18].setName("SP-LN");
  (*M)[19].setName("SN-LN");
  for(int32 x = 1; x <= 15; x++) { (*M)[x + 19].setName(fmt::format("X{:02d}", x)); }
  for(int32 m = 2; m <= 15; m++) { (*M)[m + 33].setName(fmt::format("M{:02d}", m)); }
}
void xEntropyAritCommon::xFillDebugInfoAC(tArithModelsAC* M)
{
  if(M == nullptr) { return; }
  for(int32 k = 1; k < 64; k++)
  {
    (*M)[3*(k-1)+0].setName(fmt::format("SE-K{:02d}", k));
    (*M)[3*(k-1)+1].setName(fmt::format("S0-K{:02d}", k));
    (*M)[3*(k-1)+2].setName(fmt::format("S1-K{:02d}", k));
  }
  for(int32 x = 2; x <= 15; x++) { (*M)[x + 187].setName(fmt::format("X{:02d}-lK", x)); }
  for(int32 m = 2; m <= 15; m++) { (*M)[m + 201].setName(fmt::format("M{:02d}-lK", m)); }
  for(int32 x = 2; x <= 15; x++) { (*M)[x + 215].setName(fmt::format("X{:02d}-hK", x)); }
  for(int32 m = 2; m <= 15; m++) { (*M)[m + 229].setName(fmt::format("M{:02d}-hK", m)); }
}
#else
void xEntropyAritCommon::xFillDebugInfoDC(tArithModelsDC* /*M*/) {}
void xEntropyAritCommon::xFillDebugInfoAC(tArithModelsAC* /*<*/) {}
#endif //PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
void xEntropyAritCommon::xResetModelsDC(tArithModelsDC* M)
{
#if PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
  for(int32 i = 0; i < c_NumStatModelsDC; i++) { (*M)[i].init(); }
#else //PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
  memset((void*)M, 0, sizeof(tArithModelsDC));
#endif //PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
}
void xEntropyAritCommon::xResetModelsAC(tArithModelsAC* M)
{
#if PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
  for(int32 i = 0; i < c_NumStatModelsAC; i++) { (*M)[i].init(); }
#else //PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
  memset((void*)M, 0, sizeof(tArithModelsAC));
#endif //PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
}
bool xEntropyAritCommon::xInit(const std::vector<xJFIF::xArithCond>& ArithConds)
{
  bool Result = true;
  for(const xJFIF::xArithCond& ArithCond : ArithConds)
  {
    xJFIF::xArithCond::tClass Class = ArithCond.getClass();
    int32                     Id    = ArithCond.getIdx  ();

    switch(Class)
    {
    case xJFIF::xHuffTable::tClass::DC:
      if(m_StatsModelsDC[Id] == nullptr) { m_StatsModelsDC[Id] = new tArithModelsDC; xFillDebugInfoDC(m_StatsModelsDC[Id]); }
      m_BoundL[Id] = xCalcBoundL(ArithCond.getL());
      m_BoundU[Id] = xCalcBoundU(ArithCond.getU());
      break;
    case xJFIF::xHuffTable::tClass::AC:
      if(m_StatsModelsAC[Id] == nullptr) { m_StatsModelsAC[Id] = new tArithModelsAC; xFillDebugInfoAC(m_StatsModelsAC[Id]); }
      m_Kx[Id] = ArithCond.getKx();
      break;
    default: Result = false; break;
    }
  }
  return Result;
}
void xEntropyAritCommon::xUnInit()
{
  for(int32 Id = 0; Id < xJPEG_Constants::c_MaxArithSets; Id++)
  {
    if(m_StatsModelsDC[Id] != nullptr) { delete m_StatsModelsDC[Id]; m_StatsModelsDC[Id] = nullptr; }
    if(m_StatsModelsAC[Id] != nullptr) { delete m_StatsModelsAC[Id]; m_StatsModelsAC[Id] = nullptr; }
  }
}
void xEntropyAritCommon::xReset()
{
  m_LastDC      = c_InitLastDCs;
  m_LastDeltaDC = c_InitLastDCs;

  for(int32 Id = 0; Id < xJPEG_Constants::c_MaxArithSets; Id++)
  {
    if(m_StatsModelsDC[Id] != nullptr) { xResetModelsDC(m_StatsModelsDC[Id]); }
    if(m_StatsModelsAC[Id] != nullptr) { xResetModelsAC(m_StatsModelsAC[Id]); }
  }
}

xEntropyAritCommon::eCategoryLastDeltaDC xEntropyAritCommon::categorizeLastDeltsDC(int32 LastDeltaDC, int32 ArithCondIdDC)
{
  if(LastDeltaDC <= m_BoundL[ArithCondIdDC] && LastDeltaDC >= -m_BoundL[ArithCondIdDC])
  {
    return eCategoryLastDeltaDC::Zero;
  }
  else
  {
    if(LastDeltaDC > 0)
    {
      if(LastDeltaDC <= m_BoundU[ArithCondIdDC])
      {
        return eCategoryLastDeltaDC::SmallPos;
      }
      else
      {
        return eCategoryLastDeltaDC::LargePos;
      }
    }
    else
    {
      if(LastDeltaDC >= -m_BoundU[ArithCondIdDC])
      {
        return eCategoryLastDeltaDC::SmallNeg;
      }
      else
      {
        return eCategoryLastDeltaDC::LargeNeg;
      }
    }
  }
}

//=====================================================================================================================================================================================
// xEntropyAritDecoder
//=====================================================================================================================================================================================
int32 xEntropyAritDecoder::xDecodeAbs_ref(xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX, bool isDC)
{
  int32 M = 1;

  //decode magnitude log2 - 
  uint32 D = m_ArithmCore.decodeBinMP(*CurrModel);
  if(D == 0) { goto labelFIN; }

  M = 2;
  if(isDC) { CurrModel = ModelsX + 0; } //S = X1 
  D = m_ArithmCore.decodeBinMP(*CurrModel);
  if(D == 0) { goto labelFIN; }

  M = 4;
  CurrModel = ModelsX + (isDC ? 1 : 0); //S = X2

  while(1)
  {
    D = m_ArithmCore.decodeBinMP(*CurrModel);
    if(D == 0) { goto labelFIN; }
    M = M << 1;
    CurrModel = CurrModel + 1; //S = S + 1
  }

labelFIN:
  M = M >> 1;
  int32 Sz = M;

  //move from Xn model to Mn model
  CurrModel = CurrModel + 14;

  //decode magnitude bits - 
  do
  {
    M = M >> 1;
    if(M == 0) { break; }

    D = m_ArithmCore.decodeBinMP(*CurrModel);

    if(D != 0)
    {
      Sz = M | Sz;
    }

  } while(1);

  return Sz;
}
int32 xEntropyAritDecoder::xDecodeDC(eCmp Cmp, int32 ArithCondIdDC)
{  
  eCategoryLastDeltaDC Category = categorizeLastDeltsDC(m_LastDeltaDC[(int32)Cmp], ArithCondIdDC); //select variant based on last DC
  xArithmCoreModel* ModelsCmp = m_StatsModelsDC[ArithCondIdDC]->data(); //get models for selected component
  xArithmCoreModel* ModelsSx = ModelsCmp + 4 * (int32)Category; //get pointer to first 4 models (S0, SS, SP, SN)

  //decode DC == 0
  int32 NonZeroDeltaDC = m_ArithmCore.decodeBinMP(ModelsSx[0]); //model "S0"
  if(!NonZeroDeltaDC)
  {
    m_LastDeltaDC[(int32)Cmp] = 0;
    return m_LastDC[(int32)Cmp];
  }
  else
  {
    //decode sign
    uint32 SignBin = m_ArithmCore.decodeBinMP(ModelsSx[1]); //model "SS"
    xArithmCoreModel* Model = ModelsSx + 2 + SignBin; //select model SP,SN for AbsDeltaDC > 1
    int32 AbsDeltaDC = xDecodeAbs_ref(Model, ModelsCmp + 20, true) + 1;
    int32 DeltaDC    = SignBin == 0 ? AbsDeltaDC : -AbsDeltaDC;
    m_LastDeltaDC[(int32)Cmp] = (int16)DeltaDC;
    int32 DC = (int32)m_LastDC[(int32)Cmp] + DeltaDC;
    m_LastDC[(int32)Cmp] = (int16)DC;
    return DC;
  }
}
void xEntropyAritDecoder::xDecodeAC(int16* ScanCoeff, int32 ArithCondIdAC)
{  
  xArithmCoreModel* ModelsCmp = m_StatsModelsAC[ArithCondIdAC]->data(); //get models for selected component

  for(int32 k = 1; k < 64; k++)
  {
    //get pointer to first 3 models (SE, S0, SPSN)
    xArithmCoreModel* ModelsSx = ModelsCmp + 3 * (k - 1);

    //decode EOB
    uint32 EOB = m_ArithmCore.decodeBinMP(ModelsSx[0]); //model "SE"
    if(EOB) { break; }

    //decode S0 until non-zero
    uint32 D = m_ArithmCore.decodeBinMP(ModelsSx[1]); //model "S0"
    while(D == 0)
    {
      ModelsSx += 3;
      k++;
      D = m_ArithmCore.decodeBinMP(ModelsSx[1]); //model "S0"
    }
    //decode sign
    uint32 SignBin = m_ArithmCore.decodeBinEP();

    xArithmCoreModel* ModelsXM = ModelsCmp + (k <= m_Kx[ArithCondIdAC] ? 189 : 217); //calculate location of first X model (X2) 
    int32 AbsAC = xDecodeAbs_ref(ModelsSx + 2, ModelsXM, false) + 1;
    int32 AC = SignBin == 0 ? AbsAC : -AbsAC;
    ScanCoeff[k] = (int16)AC;
  }
}
void xEntropyAritDecoder::StartSlice(xByteBuffer* ByteBuffer)
{
  m_ArithmCore.bindByteBuffer(ByteBuffer);
  m_ArithmCore.start();
  xReset();
}
void xEntropyAritDecoder::FinishSlice()
{
  m_ArithmCore.finish();
  m_ArithmCore.unbindByteBuffer();
}
void xEntropyAritDecoder::DecodeBlock(int16* ScanCoeff, eCmp Cmp, int32 ArithCondIdDC, int32 ArithCondIdAC)
{
  memset(ScanCoeff, 0, xJPEG_Constants::c_BlockArea * sizeof(int16));
  ScanCoeff[0] = (int16)xDecodeDC(Cmp, ArithCondIdDC);
  xDecodeAC(ScanCoeff, ArithCondIdAC);
}

//=====================================================================================================================================================================================
// xEntropyAritEncoderBase
//=====================================================================================================================================================================================
template<class tArithmCore> void xEntropyAritEncoderBase<tArithmCore>::xEncodeAbs_ref(int32 Value, xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX, bool isDC)
{
  int32 M = 1;

  //encode magnitude log2 - Encode_log2_Sz
  if(Value < M) { goto labelFIN; }

  m_ArithmCore.encodeBinMP(1, *CurrModel);
  M = 2;
  if(isDC) { CurrModel = ModelsX + 0; } //S = X1  

  if(Value < M) { goto labelFIN; }

  m_ArithmCore.encodeBinMP(1, *CurrModel);
  M = 4;
  CurrModel = ModelsX + (isDC ? 1 : 0); //S = X2

  while(1)
  {
    if(Value < M) { goto labelFIN; }

    m_ArithmCore.encodeBinMP(1, *CurrModel);
    M = M << 1;
    CurrModel = CurrModel + 1; //S = S + 1
  }

labelFIN:
  m_ArithmCore.encodeBinMP(0, *CurrModel);
  M = M >> 1;

  //move from Xn model to Mn model
  CurrModel = CurrModel + 14;

  //encode magnitude bits - Encode_Sz-bits
  do
  {
    M = M >> 1;
    if(M == 0) { break; }

    int32 T = M & Value ? 1 : 0;

    m_ArithmCore.encodeBinMP(T, *CurrModel);

  } while(1);

}
template<class tArithmCore> void xEntropyAritEncoderBase<tArithmCore>::xEncodeAbsDC(int32 Value, xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX)
{
  int32 M = 0;

  //encode magnitude log2 - Encode_log2_Sz
  if(Value > 0)
  {
    m_ArithmCore.encodeBinMP(1, *CurrModel);
    M = 1;
    CurrModel = ModelsX + 0; //S = X1  
    int32 TmpValue = Value;

    while(TmpValue >>= 1)
    {
      m_ArithmCore.encodeBinMP(1, *CurrModel);
      M = M << 1;
      CurrModel = CurrModel + 1; //S = S + 1
    }
  }
  m_ArithmCore.encodeBinMP(0, *CurrModel);

  //encode magnitude bits - Encode_Sz-bits
  CurrModel = CurrModel + 14; //move from Xn model to Mn model
  while(M >>= 1)
  {
    m_ArithmCore.encodeBinMP(M & Value ? 1 : 0, *CurrModel);
  }
}
template<class tArithmCore> void xEntropyAritEncoderBase<tArithmCore>::xEncodeAbsAC(int32 Value, xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX)
{
  int32 M = 0;
  //encode magnitude log2 - Encode_log2_Sz
  if(Value > 0)
  {
    m_ArithmCore.encodeBinMP(1, *CurrModel);
    M = 1;    
    int32 TmpValue = Value;

    if(TmpValue >>= 1)
    {
      m_ArithmCore.encodeBinMP(1, *CurrModel);
      M = M << 1;
      CurrModel = ModelsX + 0; //S = X2  

      while(TmpValue >>= 1)
      {
        m_ArithmCore.encodeBinMP(1, *CurrModel);
        M = M << 1;
        CurrModel = CurrModel + 1; //S = S + 1
      }
    }
  }
  m_ArithmCore.encodeBinMP(0, *CurrModel);

  //encode magnitude bits - Encode_Sz-bits
  CurrModel = CurrModel + 14; //move from Xn model to Mn model
  while(M >>= 1)
  {
    m_ArithmCore.encodeBinMP(M & Value ? 1 : 0, *CurrModel);
  }
}
template<class tArithmCore> void xEntropyAritEncoderBase<tArithmCore>::xEncodeDC(int32 DC, eCmp Cmp, int32 ArithCondIdDC)
{
  int32 DeltaDC = DC - (int32)m_LastDC[(int32)Cmp];
  m_LastDC[(int32)Cmp] = (int16)DC;
  
  eCategoryLastDeltaDC Category  = categorizeLastDeltsDC(m_LastDeltaDC[(int32)Cmp], ArithCondIdDC); //select variant based on last DC
  xArithmCoreModel*    ModelsCmp = m_StatsModelsDC[ArithCondIdDC]->data(); //get models for selected component
  xArithmCoreModel*    ModelsSx  = ModelsCmp + 4 * (int32)Category; //get pointer to first 4 models (S0, SS, SP, SN)

  if(DeltaDC == 0)
  {
    //encode DC == 0
    m_ArithmCore.encodeBinMP(0, ModelsSx[0]); //model "S0"
  }
  else //DeltaDC != 0
  {
    //encode DC != 0
    m_ArithmCore.encodeBinMP(1, ModelsSx[0]); //model "S0"

    //encode sign
    uint32 SignBin = DeltaDC < 0 ? 1 : 0;
    m_ArithmCore.encodeBinMP(SignBin, ModelsSx[1]); //model "SS"

    
    xArithmCoreModel* ModelS1  = ModelsSx + 2 + SignBin; //select model SP,SN for AbsDeltaDC > 1
    xArithmCoreModel* ModelsXM = ModelsCmp + 20; //calculate location of first X model (X1) 
    xEncodeAbsDC(xAbs(DeltaDC) - 1, ModelS1, ModelsXM);
  }

  m_LastDeltaDC[(int32)Cmp] = (int16)DeltaDC;
}
template<class tArithmCore> void xEntropyAritEncoderBase<tArithmCore>::xEncodeAC(const int16* ScanCoeff, int32 ArithCondIdAC)
{  
  xArithmCoreModel* ModelsCmp = m_StatsModelsAC[ArithCondIdAC]->data(); //get models for selected component

  const int32 LastNonZero = xEntropyUtils::findLastNonZero(ScanCoeff);

  for(int32 k = 1; k <= LastNonZero; k++)
  {
    //get pointer to first 3 models (SE, S0, SPSN)
    xArithmCoreModel* ModelsSx = ModelsCmp + 3 * (k-1);

    //encode EOB
    m_ArithmCore.encodeBinMP(0, ModelsSx[0]); //model "SE"

    int32 Coeff = ScanCoeff[k];
    while(Coeff == 0)
    {
      //encode coeff[k]==0 until non-zero      
      m_ArithmCore.encodeBinMP(0, ModelsSx[1]); //model "S0"
      ModelsSx += 3;
      Coeff = ScanCoeff[++k];
    }
    
    //encode coeff[k]!=0
    m_ArithmCore.encodeBinMP(1, ModelsSx[1]); //model "S0"
    //encode sign
    uint32 SignBin = Coeff < 0 ? 1 : 0;
    m_ArithmCore.encodeBinEP(SignBin);
    
    xArithmCoreModel* ModelsXM = ModelsCmp + (k <= m_Kx[ArithCondIdAC] ? 189 : 217); //calculate location of first X model (X2) 
    xEncodeAbsAC(xAbs(Coeff)-1, ModelsSx+2, ModelsXM);    
  }
  if(LastNonZero < 63)
  {
    //get pointer to first 3 models (SE, S0, SPSN)
    xArithmCoreModel* ModelsSx = ModelsCmp + 3 * (LastNonZero);
    //encode EOB
    m_ArithmCore.encodeBinMP(1, ModelsSx[0]); //model "SE"
  }
}

//=====================================================================================================================================================================================
// xEntropyAritEncoder
//=====================================================================================================================================================================================
void xEntropyAritEncoder::StartSlice(xByteBuffer* ByteBuffer)
{
  m_ArithmCore.bindByteBuffer(ByteBuffer);
  m_ArithmCore.start();
  xReset();
}
void xEntropyAritEncoder::FinishSlice()
{
  m_ArithmCore.finish();
  m_ArithmCore.unbindByteBuffer();
}
void xEntropyAritEncoder::EncodeBlock(const int16* ScanCoeff, eCmp Cmp, int32 ArithCondIdDC, int32 ArithCondIdAC)
{
  xEncodeDC(ScanCoeff[0], Cmp, ArithCondIdDC);
  xEncodeAC(ScanCoeff   ,      ArithCondIdAC);
}

//=====================================================================================================================================================================================
// xEntropyAritEstimator
//=====================================================================================================================================================================================
void xEntropyAritEstimator::InitDC(int32 L, int32 U, int32 ArithCondIdDC)
{
  m_BoundL[ArithCondIdDC] = xCalcBoundL(L);
  m_BoundU[ArithCondIdDC] = xCalcBoundU(U);
}
void xEntropyAritEstimator::InitAC(int32 Kx, int32 ArithCondIdAC)
{
  m_Kx[ArithCondIdAC] = Kx;
}
void xEntropyAritEstimator::EncodeBlock(const int16* ScanCoeff, eCmp Cmp, int32 ArithCondIdDC, int32 ArithCondIdAC)
{
  int64 FracNumBitsBeg = m_ArithmCore.getFracNumBits();
  xEncodeDC(ScanCoeff[0], Cmp, ArithCondIdDC);
  xEncodeAC(ScanCoeff   ,      ArithCondIdAC);
  int64 FracNumBitsEnd = m_ArithmCore.getFracNumBits();
  m_FracBits[(int32)Cmp] += FracNumBitsEnd - FracNumBitsBeg;
}
int64 xEntropyAritEstimator::EncodeAndGetFracBitsDC(int32 DC, eCmp Cmp, int32 ArithCondIdDC)
{
  int64 FracNumBitsBeg = m_ArithmCore.getFracNumBits();
  xEncodeDC(DC, Cmp, ArithCondIdDC);
  int64 FracNumBitsEnd = m_ArithmCore.getFracNumBits();
  return FracNumBitsEnd - FracNumBitsBeg;
}
int64 xEntropyAritEstimator::EncodeAndGetFracBitsAC(const int16* ScanCoeff, int32 ArithCondIdAC)
{
  int64 FracNumBitsBeg = m_ArithmCore.getFracNumBits();
  xEncodeAC(ScanCoeff, ArithCondIdAC);
  int64 FracNumBitsEnd = m_ArithmCore.getFracNumBits();
  return FracNumBitsEnd - FracNumBitsBeg;
}
int64V4 xEntropyAritEstimator::getNumBits() const
{
  int64V4 Bits;
  for(int32 i = 0; i < xJPEG_Constants::c_MaxComponents; i++)
  {
    Bits[i] = (m_FracBits[i] + xArithmCoreCmn::c_FracBitsRoundUp) >> xArithmCoreCmn::c_FracBitsShift;
  }
  return Bits;
}
void xEntropyAritEstimator::storeStateAC(xEntropyAritEstStateAC& State, int32 ArithCondIdAC)
{
  memcpy(&State.StatsModelsAC, m_StatsModelsAC[ArithCondIdAC], sizeof(tArithModelsAC));
  State.ArithmCore = m_ArithmCore;
}
void xEntropyAritEstimator::loadStateAC(xEntropyAritEstStateAC& State, int32 ArithCondIdAC)
{
  memcpy(m_StatsModelsAC[ArithCondIdAC], &State.StatsModelsAC, sizeof(tArithModelsAC));
  m_ArithmCore = State.ArithmCore;
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG