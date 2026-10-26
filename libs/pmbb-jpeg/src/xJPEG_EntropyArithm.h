/*
    SPDX-FileCopyrightText: 2024-2025 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJPEG_Constants.h"
#include "xJFIF.h"
#include "xJPEG_Arithm.h"
#include "xMemoryAlign.h"
#include "xVec.h"
#include <array>

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

/*
ArithModelsDC
0  SO  DC !=0 category=Zero     
1  SS  DC sign
2  SP  DC > 1
3  SN  DC <-1
4  SO  DC !=0 category=SmallPos
5  SS  DC sign
6  SP  DC > 1
7  SN  DC <-1
8  SO  DC !=0 category=SmallNeg
9  SS  DC sign
10 SP  DC > 1
11 SN  DC <-1
12 SO  DC !=0 category=LargePos
13 SS  DC sign
14 SP  DC > 1
15 SN  DC <-1
16 SO  DC!=0 category=LargeNeg
17 SS  DC sign
18 SP  DC > 1
19 SN  DC <-1
20 X1  Log2 = AbsDC < 2^1
   .....
34 X15 Log2 = AbsDC < 2^15
35 M2  Value = Magnitude bits if AbsDC < 2^2
   .....
48 M15 Value = Magnitude bits if AbsDC < 2^15

ArithModelsAC
  0 SE   AC[1]
  1 S0   
  2 SPSN 
  3 SE   AC[2]
  4 S0
  5 SPSN
    .....
186 SE   AC[63]
187 S0
188 SPSN
189 X2  Log2 = AbsAC < 2^2 category K<=Kx
   .....
202 X15 Log2 = AbsAC < 2^15
203 M2  Value = Magnitude bits if AbsDC < 2^2
   .....
216 M15 Value = Magnitude bits if AbsDC < 2^15
217 X2  Log2 = AbsAC < 2^1 category K>Kx
   .....
231 X15 Log2 = AbsAC < 2^15
232 M2  Value = Magnitude bits if AbsDC < 2^2
   .....
245 M15 Value = Magnitude bits if AbsDC < 2^15

*/

//=====================================================================================================================================================================================

class xEntropyAritCommon
{
public:
  enum class eCategoryLastDeltaDC
  {
    Zero     = 0,
    SmallPos = 1,
    SmallNeg = 2,
    LargePos = 3,
    LargeNeg = 4,
  };

  static constexpr int32 c_NumStatModelsDC = 64 ; //actualy 49 , but rounded up to nearest power of two
  static constexpr int32 c_NumStatModelsAC = 256; //actualy 245, but rounded up to nearest power of two

  struct PMBB_ALIGN_CACHE tArithModelsDC : std::array<xArithmCoreModel, c_NumStatModelsDC> {}; //  using tArithModelsDC = std::array<xArithmCoreModel, c_NumStatModelsDC>;
  struct PMBB_ALIGN_CACHE tArithModelsAC : std::array<xArithmCoreModel, c_NumStatModelsAC> {}; //  using tArithModelsAC = std::array<xArithmCoreModel, c_NumStatModelsAC>;

protected:
  using tLDCs = std::array<int16, xJPEG_Constants::c_MaxComponents>;
  static constexpr tLDCs c_InitLastDCs = { 0, 0, 0, 0 };

  static void xFillDebugInfoDC(tArithModelsDC* M);
  static void xFillDebugInfoAC(tArithModelsAC* M);
  static void xResetModelsDC  (tArithModelsDC* M);
  static void xResetModelsAC  (tArithModelsAC* M);

  static int32 xCalcBoundL(int32 L) { return L == 0 ? 0 : 1 << (L - 1); } //F.1.4.4.1.2
  static int32 xCalcBoundU(int32 U) { return 1 << U                   ; } //F.1.4.4.1.2

  bool xInit  (const std::vector<xJFIF::xArithCond>& ArithConds);
  void xUnInit();
  void xReset ();

protected:
  tLDCs m_LastDC     ;
  tLDCs m_LastDeltaDC;

  std::array<tArithModelsDC*, xJPEG_Constants::c_MaxArithSets> m_StatsModelsDC = { nullptr, nullptr, nullptr, nullptr };
  std::array<tArithModelsAC*, xJPEG_Constants::c_MaxArithSets> m_StatsModelsAC = { nullptr, nullptr, nullptr, nullptr };

  std::array<int32, xJPEG_Constants::c_MaxArithSets> m_BoundL;
  std::array<int32, xJPEG_Constants::c_MaxArithSets> m_BoundU;
  std::array<int32, xJPEG_Constants::c_MaxArithSets> m_Kx    ;

  eCategoryLastDeltaDC categorizeLastDeltsDC(int32 LastDeltaDC, int32 ArithCondIdDC);
};

//=====================================================================================================================================================================================

class PMBB_ALIGN_CACHE xEntropyAritDecoder : public xEntropyAritCommon
{
protected:
  xArithmCoreDec m_ArithmCore;

  int32 xDecodeAbs_ref(xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX, bool isDC);

  int32 xDecodeAbsDC(xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX);
  int32 xDecodeAbsAC(xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX);

public:
  int32 xDecodeDC(                  eCmp Cmp, int32 ArithCondIdDC);
  void  xDecodeAC(int16* ScanCoeff,           int32 ArithCondIdAC); //Decode_AC_coefficients (Figure F.20 - Procedure for decoding the AC coefficients)

public:
  bool  Init  (const std::vector<xJFIF::xArithCond>& ArithConds) { return xInit(ArithConds); }
  void  UnInit(                                                ) { xUnInit()               ; }

  void  StartSlice (xByteBuffer* ByteBuffer);
  void  FinishSlice();
  void  DecodeBlock(int16* ScanCoeff, eCmp Cmp, int32 ArithCondIdDC, int32 ArithCondIdAC);
};

//=====================================================================================================================================================================================

template<class tArithmCore> class xEntropyAritEncoderBase : public xEntropyAritCommon
{
protected:
  tArithmCore m_ArithmCore;

  void  xEncodeAbs_ref(int32 Value, xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX, bool isDC);
  void  xEncodeAbsDC(int32 Value, xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX);
  void  xEncodeAbsAC(int32 Value, xArithmCoreModel* CurrModel, xArithmCoreModel* ModelsX);

public:
  void  xEncodeDC   (int32 DC              , eCmp Cmp, int32 ArithCondIdDC);
  void  xEncodeAC   (const int16* ScanCoeff,           int32 ArithCondIdAC);

public:
  bool  Init  (const std::vector<xJFIF::xArithCond>& ArithConds) { return xInit(ArithConds); }
  void  UnInit(                                                ) { xUnInit()               ; }
};

//=====================================================================================================================================================================================

class PMBB_ALIGN_CACHE xEntropyAritEncoder : public xEntropyAritEncoderBase<xArithmCoreEnc>
{
public:
  void  StartSlice (xByteBuffer* ByteBuffer);
  void  FinishSlice();

  void  EncodeBlock(const int16* ScanCoeff, eCmp Cmp, int32 ArithCondIdDC, int32 ArithCondIdAC);
};

//=====================================================================================================================================================================================

struct xEntropyAritEstStateAC
{
public:
  xEntropyAritCommon::tArithModelsAC StatsModelsAC;
  xArithmCoreCnt                     ArithmCore   ;
};

//=====================================================================================================================================================================================

class PMBB_ALIGN_CACHE xEntropyAritEstimator : public xEntropyAritEncoderBase<xArithmCoreCnt>
{
protected:
  std::array<int64, xJPEG_Constants::c_MaxComponents> m_FracBits;

public:
  void  InitDC(int32 L, int32 U, int32 ArithCondIdDC);
  void  InitAC(int32 Kx        , int32 ArithCondIdAC);

  void  StartSlice () { m_ArithmCore.start (); xReset(); m_FracBits.fill(0); }
  void  FinishSlice() { m_ArithmCore.finish();                               }

  void  EncodeBlock(const int16* ScanCoeff, eCmp Cmp, int32 ArithCondIdDC, int32 ArithCondIdAC);

  void  EncodeDC(int32 DC, eCmp Cmp    , int32 ArithCondIdDC) { xEncodeDC(DC, Cmp  , ArithCondIdDC); }
  void  EncodeAC(const int16* ScanCoeff, int32 ArithCondIdAC) { xEncodeAC(ScanCoeff, ArithCondIdAC); }

  int64 EncodeAndGetFracBitsDC(int32 DC, eCmp Cmp    , int32 ArithCondIdDC);
  int64 EncodeAndGetFracBitsAC(const int16* ScanCoeff, int32 ArithCondIdAC);

  int64V4 getNumBits    () const;
  uint32  getNumBitsCore() const { return m_ArithmCore.getIntNumBits(); }

  void  storeStateAC(xEntropyAritEstStateAC& State, int32 ArithCondIdAC);
  void  loadStateAC (xEntropyAritEstStateAC& State, int32 ArithCondIdAC);
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG