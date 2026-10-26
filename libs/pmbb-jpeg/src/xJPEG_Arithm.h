/*
    SPDX-FileCopyrightText: 2024-2025 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJPEG_Constants.h"
#include "xByteBuffer.h"
#include <array>

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

#define PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION 0
#define PMBB_JPEG_ARITHM_AS_IN_STANDARD         0
#define PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO       0

//=====================================================================================================================================================================================

class xArithmCoreModel
{
public:
  static constexpr int32 c_NumProbIdx = 113;

protected:
  static const uint8 c_NextState[2*c_NumProbIdx][2]; //[m_State][BinValue]
#if PMBB_JPEG_ARITHM_AS_IN_STANDARD
  static const uint8 c_NextIndex[c_NumProbIdx][3];
#endif //PMBB_JPEG_ARITHM_AS_IN_STANDARD

protected:
  uint8 m_State = 0;   
#if PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
  std::string m_Name;
#endif 

public:
  void init      (                          ) { m_State = 0; }
  void set       (uint32 ProbIdx, uint32 MPS) { assert(ProbIdx < c_NumProbIdx && MPS <= 1); m_State = (uint8)((ProbIdx << 1) | MPS); }
  void setProbIdx(uint32 ProbIdx            ) { assert(ProbIdx < c_NumProbIdx            ); m_State = (m_State & 0x01) | ((uint8)ProbIdx << 1); }
  void setMPS    (                uint32 MPS) { assert(                          MPS <= 1); m_State = (m_State & 0xFE) | ((uint8)MPS         ); }
  
  uint8 getProbIdx() const { return m_State >> 1  ; }
  uint8 getMPS    () const { return m_State & 0x01; }

  void  updateTab(uint32 BinVal) { m_State = c_NextState[m_State][BinVal]; }

#if PMBB_JPEG_ARITHM_AS_IN_STANDARD
  //slow update path taken directly from standard
  void  updateStd   (uint32 BinVal);
  void  updateStdMPS();
  void  updateStdLPS();
#endif //PMBB_JPEG_ARITHM_AS_IN_STANDARD

#if PMBB_JPEG_ARITHM_MODEL_DEBUG_INFO
  void setName(const std::string Name) { m_Name = Name; }
#endif
};

//=====================================================================================================================================================================================

class xArithmCoreCmn
{
public:
  static constexpr int32 c_FracBitsShift   = 15;
  static constexpr int32 c_FracBitsRoundUp = ((1 << c_FracBitsShift) - 1);

  static const uint16 c_Qe         [  xArithmCoreModel::c_NumProbIdx];
  static const uint32 c_EntropyBits[2*xArithmCoreModel::c_NumProbIdx]; //EB = round((-log2(Probability)) << c_FracBitsShift)
  
  //equal probability
  static constexpr uint16 c_EqualProbQe          = 0x5A1D;
  static constexpr uint16 c_EqualProbMPS         = 0;
  static constexpr flt32  c_EqualProbProbability = 0.5;
  static constexpr uint32 c_EqualProbEntropyBits = 0x08000;

protected:
  xByteBuffer* m_ByteBuffer = nullptr;

  uint32 m_C ; //D.1.3 - in decoder it is combined C and Cx
  int32  m_A ; //D.1.3 
  int32  m_CT; //bit shift counter, determines when next byte will be written

public:
  xByteBuffer* getByteBuffer   (                       ) { return m_ByteBuffer; }
  bool         bindByteBuffer  (xByteBuffer* ByteBuffer) { if(m_ByteBuffer != nullptr) { return false  ; } else { m_ByteBuffer = ByteBuffer; return true; } }
  xByteBuffer* unbindByteBuffer(                       ) { if(m_ByteBuffer == nullptr) { return nullptr; } else { xByteBuffer* Tmp = m_ByteBuffer; m_ByteBuffer = nullptr; return Tmp; } }

  //for testing purposes
  uint32 getC () const { return m_C ; }
  uint32 getA () const { return m_A ; }
  uint32 getCT() const { return m_CT; }
};

//=====================================================================================================================================================================================

class xArithmCoreDec : public xArithmCoreCmn
{
// C here is : C = [Cx Cx Clow 00]
public:
  void   start ();
  void   finish();
#if PMBB_JPEG_ARITHM_AS_IN_STANDARD
  uint32 decodeBinMP_as_in_standard(xArithmCoreModel& CtxModel);
#endif //PMBB_JPEG_ARITHM_AS_IN_STANDARD
  uint32 decodeBinMP(xArithmCoreModel& CtxModel);
  uint32 decodeBinEP();

protected:
#if PMBB_JPEG_ARITHM_AS_IN_STANDARD
  uint32 xCond_LPS_exchange(xArithmCoreModel& CtxModel); //"as in standard" path
  uint32 xCond_MPS_exchange(xArithmCoreModel& CtxModel); //"as in standard" path
#endif //PMBB_JPEG_ARITHM_AS_IN_STANDARD
  void   xRenorm();
  void   xByteIn();
};

//=====================================================================================================================================================================================

class xArithmCoreEnc : public xArithmCoreCmn
{
  int32 m_SC; // (ST) counter for stacked 0xFF values which might overflow 
  int32 m_ZC; // counter for pending 0x00 output values which might be discarded at the end ("Pacman" termination)
  int32 m_B ; // buffer for most recent output byte != 0xFF

public:
  void start ();
  void finish();
#if PMBB_JPEG_ARITHM_AS_IN_STANDARD
  void encodeBinMP_as_in_standard(uint32 BinValue, xArithmCoreModel& CtxModel);
#endif //PMBB_JPEG_ARITHM_AS_IN_STANDARD
  void encodeBinMP(uint32 BinValue, xArithmCoreModel& CtxModel); //simplified
  void encodeBinEP(uint32 BinValue                           ); //equal probability

  //for testing purposes
  uint32 getST() const { return m_SC; } 

protected:
  void xCode0  (xArithmCoreModel& Ctx); //"as in standard" path - D.1.4 Code_1(S) and Code_0(S) procedures
  void xCode1  (xArithmCoreModel& Ctx); //"as in standard" path - D.1.4 Code_1(S) and Code_0(S) procedures
  void xCodeMPS(xArithmCoreModel& Ctx); //"as in standard" path 
  void xCodeLPS(xArithmCoreModel& Ctx); //"as in standard" path
  void xRenorm   (); //D.1.6 Renormalization in the encoder --> Figure D.7 - Encoder renormalization procedure
  void xOutput00s();
  void xOutputFFs();
};

//=====================================================================================================================================================================================

class xArithmCoreCnt
{
protected:
  int32  m_A           ; //D.1.3 
  uint64 m_FracBits = 0;

public:
  inline void start         () { m_A = 0x10000; m_FracBits = 0; }
  inline void finish        () { }

  inline void copyStateFrom(const xArithmCoreCnt* Src) { ::memcpy((void*)this, (void*)Src, sizeof(xArithmCoreCnt)); }

  void        encodeBinMP   (uint32 BinValue, xArithmCoreModel& CtxModel);
  void        encodeBinEP   (uint32 BinValue);

  inline void   resetNumBits  ()       { m_FracBits = 0; }
  inline uint64 getFracNumBits() const { return m_FracBits; }
  inline uint32 getIntNumBits () const { return (uint32)((m_FracBits + xArithmCoreCmn::c_FracBitsRoundUp) >> xArithmCoreCmn::c_FracBitsShift); }

  //for testing purposes  
  uint32 getA () const { return m_A; }

protected:
  void xPseudoRenorm(); //D.1.6 Renormalization in the encoder --> Figure D.7 - Encoder renormalization procedure
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG