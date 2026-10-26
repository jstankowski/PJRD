/*
    SPDX-FileCopyrightText: 2024-2025 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "xJPEG_EntropyArithm.h"
#include "xJPEG_TestUtils.h"
#include "xTestUtils.h"
#include "xMemory.h"
#include <array>

using namespace PMBB_NAMESPACE;
using namespace PMBB_NAMESPACE::JPEG;

//===============================================================================================================================================================================================================

static constexpr int32 BA = xJPEG_Constants::c_BlockArea;

//===============================================================================================================================================================================================================

void testEntropyArithmDC()
{
  xByteBuffer         Buff(32*1024*1024);
  xEntropyAritEncoder Enc; 
  xEntropyAritDecoder Dec; 
  std::vector<xJFIF::xArithCond> ACs = xJFIF::xArithCond::createDefaultArithCond(true);

  Enc.Init(ACs);
  Dec.Init(ACs);

  //encode  
  Enc.StartSlice(&Buff);
  for(int32 i = 0; i < 256; i++)
  {
    int32 RefDC = i;
    Enc.xEncodeDC(0       , eCmp::LM, 0); //0
    Enc.xEncodeDC(RefDC   , eCmp::LM, 0); //1
    Enc.xEncodeDC(RefDC   , eCmp::LM, 0); //2
    Enc.xEncodeDC(0       , eCmp::LM, 0); //3
    Enc.xEncodeDC(RefDC>>2, eCmp::LM, 0); //4
    Enc.xEncodeDC(RefDC>>1, eCmp::LM, 0); //5
    Enc.xEncodeDC(RefDC   , eCmp::LM, 0); //6
  }
  Enc.FinishSlice();

  //decode
  Dec.StartSlice(&Buff);
  for(int32 i = 0; i < 256; i++)
  {
    int32 RefDC  = i;
    int32 DecDC0 = Dec.xDecodeDC(eCmp::LM, 0);
    int32 DecDC1 = Dec.xDecodeDC(eCmp::LM, 0);
    int32 DecDC2 = Dec.xDecodeDC(eCmp::LM, 0);
    int32 DecDC3 = Dec.xDecodeDC(eCmp::LM, 0);
    int32 DecDC4 = Dec.xDecodeDC(eCmp::LM, 0);
    int32 DecDC5 = Dec.xDecodeDC(eCmp::LM, 0);
    int32 DecDC6 = Dec.xDecodeDC(eCmp::LM, 0);
    CHECK(DecDC0 == 0       );
    CHECK(DecDC1 == RefDC   );
    CHECK(DecDC2 == RefDC   );
    CHECK(DecDC3 == 0       );
    CHECK(DecDC4 == RefDC>>2);
    CHECK(DecDC5 == RefDC>>1);
    CHECK(DecDC6 == RefDC   );
  }
  Dec.FinishSlice();
}

void testEntropyArithm()
{
  constexpr int32 NumIters = 64;
  constexpr int32 NumBlock = 4 * 1024;
  constexpr int32 NumPels  = NumBlock * BA;
  constexpr int64 BuffSize = NumPels * sizeof(int16);

  int16* Src = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);
  int16* Dst = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);

  xByteBuffer Buffer ; Buffer.resize(BuffSize * 4);

  std::vector<xJFIF::xArithCond> ACs = xJFIF::xArithCond::createDefaultArithCond(true);

  xEntropyAritEncoder EntropyEnc;
  EntropyEnc.Init(ACs);
  xEntropyAritDecoder EntropyDec;
  EntropyDec.Init(ACs);

  //single coeff test
  //TODO

  //random test
  uint32 State = xTestUtils::c_XorShiftSeed;
  for(int32 j = 0; j < NumIters; j++)
  {
    //generate
    for(int32 i = 0; i < NumBlock; i++) { State = fillRandomTransformCoeffsBlock(Src + (i * BA), State); }

    for(int32 c = 0; c <= 1; c++)
    {
      //encode
      Buffer.reset();

      EntropyEnc.StartSlice(&Buffer);
      for(int32 i = 0; i < NumBlock; i++) { EntropyEnc.EncodeBlock(Src + (i * BA), eCmp(c), c, c); }
      EntropyEnc.FinishSlice();

      //decode
      EntropyDec.StartSlice(&Buffer);
      for(int32 i = 0; i < NumBlock; i++) { EntropyDec.DecodeBlock(Dst + (i * BA), eCmp(c), c, c); }
      EntropyDec.FinishSlice();

      //compare
      CHECK(xTestUtils::isSameBuffer(Dst, Src, NumPels, true));
    }
  }

  xMemory::xAlignedFree(Src);
  xMemory::xAlignedFree(Dst);
}

void testEntropyArithmEstimator()
{
  constexpr int32 NumIters = 64;
  constexpr int32 NumBlock = 4 * 1024;
  constexpr int32 NumPels  = NumBlock * BA;
  constexpr int64 BuffSize = NumPels * sizeof(int16);

  int16* Src = (int16*)xMemory::xAlignedMallocPageAuto(BuffSize);

  xByteBuffer EntropyBuffer; EntropyBuffer.resize(BuffSize * 4);

  std::vector<xJFIF::xArithCond> ACs = xJFIF::xArithCond::createDefaultArithCond(true);

  xEntropyAritEncoder   EntropyEnc;
  xEntropyAritEstimator EntropyEst;

  //random test
  uint32 State = xTestUtils::c_XorShiftSeed;
  for(int32 j = 0; j < NumIters; j++)
  {
    //generate
    for(int32 i = 0; i < NumBlock; i++) { State = fillRandomTransformCoeffsBlock(Src + (i * BA), State); }

    for(int32 c = 0; c <= 1; c++)
    {
      EntropyEnc.Init(ACs);
      EntropyEst.Init(ACs);

      //encode
      EntropyBuffer.reset();

      EntropyEnc.StartSlice(&EntropyBuffer);
      for(int32 i = 0; i < NumBlock; i++) { EntropyEnc.EncodeBlock(Src + (i * BA), eCmp(c), c, c); }
      EntropyEnc.FinishSlice();

      //estimate
      EntropyEst.StartSlice();
      for(int32 i = 0; i < NumBlock; i++) { EntropyEst.EncodeBlock(Src + (i * BA), eCmp(c), c, c); }
      EntropyEst.FinishSlice();

      //compare
      int32 NumBitsEnc = EntropyBuffer.getDataSize() * 8;
      int32 NumBitsEst = EntropyEst.getNumBitsCore();
      int32 NumBitsDif = xAbs(NumBitsEnc - NumBitsEst);
      flt64 ErrorPerc  = 100.0 * (flt64)NumBitsDif / (flt64)NumBitsEnc;
      CHECK(ErrorPerc < 5); //expect ~3.7%
    }
  }

  xMemory::xAlignedFree(Src);
}

//===============================================================================================================================================================================================================

TEST_CASE("testEntropyArithmDC"       ) { testEntropyArithmDC       (); }
TEST_CASE("testEntropyArithm"         ) { testEntropyArithm         (); }
TEST_CASE("testEntropyArithmEstimator") { testEntropyArithmEstimator(); }

//===============================================================================================================================================================================================================

