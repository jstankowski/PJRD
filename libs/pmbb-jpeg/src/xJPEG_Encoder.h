/*
    SPDX-FileCopyrightText: 2020-2026 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#pragma once
#include "xCommonDefJPEG.h"
#include "xJPEG_CodecCommon.h"
#include "xJFIF.h"
#include "xPicYUV.h"
#include "xJPEG_Quant.h"
#include "xJPEG_EntropyHuffman.h"
#include "xJPEG_EntropyArithm.h"
#include <array>
#include <map>

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

struct xOptNode
{
  static constexpr int32 c_BA = xJPEG_Constants::c_BlockArea;

public:
  int16 m_CoeffsQuantScan[c_BA];
  flt64 m_Cost;

  xOptNode() {}

  xOptNode(const int16* CoeffsQuantScan, flt64 Cost)
  {
    memcpy(m_CoeffsQuantScan, CoeffsQuantScan, c_BA * sizeof(int16));
    m_Cost = Cost;
  }
};

//=====================================================================================================================================================================================

class xAdvancedEncoder : public xCodecImplCommon
{
public:
  using tDistBits = std::tuple<int64V4, int64V4>;

protected:
  int32 m_Quality;
  eQTLa m_QuantTabLayout = eQTLa::INVALID;

  //encoder behaviour
  bool    m_EmitAPP0        = true;
  bool    m_EmitQuantTabs   = true;
  bool    m_EmitEntropyTabs = true;
  //optimization
  bool    m_OptQuantCoeffs     = false; //Use RDQO
  eQntOpt m_OptQuantAlgorithm  = eQntOpt::INVALID;
  bool    m_ProcessZeroCoeffs  = false;
  int32   m_NumOptBestCands    = 0;
  eLmbd   m_LambdaEstMode      = eLmbd::INVALID;
  bool    m_OptEntropyTables   = false;
  int32   m_NumOptPassesBlock  = 0;
  int32   m_NumOptPassesPic    = 0;
  eCalkMd m_BlkOptDistCalcMode = eCalkMd::Exact;
  eCalkMd m_BlkOptBitsCalcMode = eCalkMd::Exact;

  //lambdas
  flt64V4 m_Lambda = { 1.0, 1.0, 1.0, 1.0 };

  //Tools
  xQuantizerSet                 m_QuantMain;
  std::map<int32, xQuantizerSet>m_QuantAuxs;

  //Huffman
  xEntropyHuffEstimator            m_EntropyHuffEst ; //no need for multiple instances - xEntropyHuffEstimator is stateless
  std::vector<xEntropyHuffCounter> m_EntropyHuffCnts;
  xHuffEncoderBank                 m_EncoderHuffBank;
  std::vector<xEntropyHuffEncoder> m_EntropyHuffEncs;

  //Arithmetic
  std::vector<xEntropyAritEstimator > m_EntropyAritEsts;
  std::vector<xEntropyAritEncoder   > m_EntropyAritEncs;
 
  //Buffers
  xPicYUV* m_PicYCbCr444 = nullptr;
  xPicYUV* m_PicYCbCr4XX = nullptr;

  int16*  m_CmpCoeffsTransOrg    [c_NC];
  int16*  m_CmpCoeffsTransRec    [c_NC];
  int16*  m_CmpCoeffsQuantScan   [c_NC];
  int16*  m_CmpCoeffsQuantScanAux[c_NC];
  int16*  m_CmpCoeffsQuantScanOpt[c_NC];
  xPicYUV m_PicRec;

  std::vector<xByteBuffer> m_EncBuffers      ;
  std::vector<xByteBuffer> m_SliceBuffers    ;
  std::vector<int32      > m_NumAlignmentBits;
  xByteBuffer              m_CombineBuffer   ;

  //Profiling
  uint64 m_Ticks_Transform = 0;
  uint64 m_Ticks_QuantScan = 0;
  uint64 m_Ticks_LambdaEst = 0;
  uint64 m_Ticks__OptQuant = 0;
  uint64 m_TicksOptEntropy = 0;
  uint64 m_TicksEntropyEnc = 0;
  uint64 m_Ticks____Tables = 0;
  uint64 m_Ticks__WriteOut = 0;

public:
  void create (int32V2 PictureSize, eCrF ChromaFormat, eEntrTp EntropyType, xThreadPool* ThreadPool = nullptr);
  void destroy();

  void setMarkerEmit(bool EmitAPP0, bool EmitQuantTabs, bool EmitEntropyTabs);
  void setOptQuant  (bool OptQuantCoeffs, eQntOpt OptQuantAlgorithm, bool ProcessZeroCoeffs, int32 NumOptBestCands, eLmbd LambdaEstMode);
  void setOptEntropy(bool OptEntropyTables);
  void setOptPass   (int32 NumBlockOptPasses, int32 NumPicOptPasses);
  void setOptClcMode(eCalkMd BlkOptDistCalcMode, eCalkMd BlkOptBitsCalcMode);

  void initBaseMarkers();
  void initQuant      (int32 Quality, eQTLa QuantTabLayout);
  void initEntropy    (int32 RestartInterval);
  
  void encode(const xPicYUV* InputPicture, xByteBuffer* OutputBuffer);

  std::string formatAndResetStats(const std::string Prefix, flt64 TicksPerMiliSec);

protected:
  void xEncodePicture(xByteBuffer* Buffer, const xPicYUV* Picture);

  //Fwd/Inv transform + Quant/InvScale
  void xFwdTransformPic(int16* CoeffsTransV[], const xPicYUV* Picture);
  void xFwdTransformSlc(int16* CoeffsTransV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_IdxBeg, int32 MCU_IdxEnd);
  void xFwdTransformMCU(int16* CoeffsTransV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx);
  void xInvTransformPic(xPicYUV* Picture, const int16* CoeffsTransV[]);
  void xInvTransformSlc(uint16* CmpPtrV[], const int32 CmpStrideV[], const int16* CoeffsTransV[], int32 MCU_IdxBeg, int32 MCU_IdxEnd);
  void xInvTransformMCU(uint16* CmpPtrV[], const int32 CmpStrideV[], const int16* CoeffsTransV[], int32 MCU_Idx);

  void        xFwdQuantScanPic(int16* CoeffQuantScanV [], const int16* CoeffTransV[]     , const xQuantizerSet& Quant                                   );
  static void xFwdQuantScanRng(int16* CoeffQuantScan    , const int16* CoeffTrans        , const xQuantizer& Quant, int32 BlockIdxBeg, int32 BlockIdxEnd);
  void        xInvScanQuantPic(int16* CoeffTransV[]     , const int16* CoeffQuantScanV[] , const xQuantizerSet& Quant                                   );
  static void xInvScanQuantRng(int16* CoeffTrans        , const int16* CoeffQuantScan    , const xQuantizer& Quant, int32 BlockIdxBeg, int32 BlockIdxEnd);

  //Lambda estimation
  int64V4 xCalcBitsHuffPic(const int16* CoeffsQuantScanV[]);
  int64V4 xCalcBitsHuffSlc(const int16* CoeffsQuantScanV[], int32 MCU_IdxFirst, int32 MCU_IdxLast);
  int64V4 xCalcBitsHuffMCU(const int16* CoeffsQuantScanV[], int32 MCU_Idx);
  int64V4 xCalcBitsAritPic(const int16* CoeffsQuantScanV[]);
  int64V4 xCalcBitsAritSlc(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast);
  void    xCalcBitsAritMCU(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_Idx);

  template<eEntrTp ET> int64V4 xtCalcBitsPic(const int16* CoeffsQuantScanV[]);
  template<eEntrTp ET> int64V4 xtCalcBitsSlc(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast);
  template<eEntrTp ET> int64V4 xtCalcBitsMCU(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_Idx);

  int64V4 xCalcBitsPic    (const int16* CoeffsQuantScanV[]);
  int64V4 xCalcDistPicSSD (const xPicYUV* Tst, const xPicYUV* Ref);
  int64V4 xEstDistPicSSD  (const int16* CoeffsTransOrgV[], const int16* CoeffsTransRecV[]);
  int64V4 xCalcDistPic    (const int16* CoeffsQuantScanV[], const xQuantizerSet& Quant, const xPicYUV* PictureRef);
  void    xEstimateLambda (const xPicYUV* Picture);

  //RDOQ
  void   xOptQuantHuffPic(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture);
  void   xOptQuantHuffSlc(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture, int32 MCU_IdxFirst, int32 MCU_IdxLast);
  void   xOptQuantHuffMCU(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 MCU_Idx);
  void   xOptQuantHuffBLK(int16* OptCoeffQuantScan, const int16* CoeffsQuantScan, const int16* CoeffsTransOrg, const uint16* SamplesOrg, eCmp CmpId, int32 LastDC);
  void   xOptQuantHuffBLK2(int16* OptCoeffQuantScan, const int16* CoeffsQuantScan, const int16* CoeffsTransOrg, const uint16* SamplesOrg, eCmp CmpId, int32 LastDC, int32 Limit);
  void   xOptQuantAritPic(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture);
  void   xOptQuantAritSlc(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture, int32 SliceIdx);
  void   xOptQuantAritMCU(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 SliceIdx, int32 MCU_Idx);
  void   xOptQuantAritBLK(int16* OptCoeffQuantScan, const int16* CoeffsQuantScan, const int16* CoeffsTransOrg, const uint16* SamplesOrg, int32 SliceIdx, eCmp CmpId);
  void   xOptQuantAritBLK2(int16* OptCoeffQuantScan, const int16* CoeffsQuantScan, const int16* CoeffsTransOrg, const uint16* SamplesOrg, int32 SliceIdx, eCmp CmpId, int32 Limit);
  
  template<eEntrTp ET> void xtOptQuantPic(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture);
  template<eEntrTp ET> void xtOptQuantSlc(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture, int32 SliceIdx);
  template<eEntrTp ET> void xtOptQuantMCU(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const uint16* CmpPtrV[], const int32 CmpStrideV[], int32 SliceIdx, int32 MCU_Idx);
  template<eEntrTp ET> void xtOptQuantBLK1(int16* OptCoeffQuantScan, const int16* CoeffsQuantScan, const int16* CoeffsTransOrg, const uint16* SamplesOrg, int32 SliceIdx, eCmp CmpId);
  template<eEntrTp ET> void xtOptQuantBLK2(int16* OptCoeffQuantScan, const int16* CoeffsQuantScan, const int16* CoeffsTransOrg, const uint16* SamplesOrg, int32 SliceIdx, eCmp CmpId, int32 Limit);

  
  uint64 xCalcDistBLK     (const int16* CoeffsQuantScan, const int16* CoeffsTransOrgScan, const uint16* SamplesOrg, const xQuantizer& Quantizer);
  uint64 xCalkExactDistBLK(const int16* CoeffsQuantScan, const uint16* SamplesOrg       , const xQuantizer& Quantizer);
  uint64 xCalkApprxDistBLK(const int16* CoeffsQuantScan, const int16* CoeffsTransOrgScan, const xQuantizer& Quantizer);
  void   xOptQuantPic     (int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture);
  
  //Huffman tables & Arithmetic conditioning optimization
  void   xOptHuffPic(const int16* CoeffsQuantScanV[]);
  void   xCntHuffPic(const int16* CoeffsQuantScanV[]);
  void   xCntHuffRng(const int16* CoeffsQuantScan, int32 CounterIdx, int32 HuffTabIdDC, int32 HuffTabIdAC, int32 BlockIdxBeg, int32 BlockIdxEnd, int32 BlocksPerSlice);
  void   xOptAritPic         (const int16* CoeffsQuantScanV[]);
  void   xCalcAritCondDC     (std::vector<int64V4>& BitsDC, const int16* CoeffsQuantScanV[]);
  void   xCalcAritCondAC     (std::vector<int64V4>& BitsAC, const int16* CoeffsQuantScanV[]);
  int64  xCalcBitsAritSliceDC(const int16* CoeffsQuantScanV[], eCmp Cmp, int32 SliceIdx, int32 U, int32 L);
  int64  xCalcBitsAritSliceAC(const int16* CoeffsQuantScanV[], eCmp Cmp, int32 SliceIdx, int32 Kx);
  void   xOptEntropyPic(const int16* CoeffsQuantScanV[]);

  //Entropy encode
  void xEncodeHuffPic(const int16* CoeffsQuantScanV[]);
  void xEncodeHuffSlc(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast);
  void xEncodeHuffChk(const int16* CoeffsQuantScanV[], int32 ChunkIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast);
  void xEncodeHuffMCU(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_Idx);
  void xEncodeAritPic(const int16* CoeffsQuantScanV[]);
  void xEncodeAritSlc(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_IdxFirst, int32 MCU_IdxLast);
  void xEncodeAritMCU(const int16* CoeffsQuantScanV[], int32 SliceIdx, int32 MCU_Idx);
  void xEncodePic    (const int16* CoeffsQuantScanV[]);

  //Writeout
  void xWritePic(xByteBuffer* OutputBuffer);
};

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG