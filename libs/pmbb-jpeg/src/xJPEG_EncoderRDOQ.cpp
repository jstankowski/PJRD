/*
    SPDX-FileCopyrightText: 2020-2026 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xJPEG_Encoder.h"
#include "xJPEG_Transform.h"
#include "xJPEG_TransformConstants.h"
#include "xJPEG_Scan.h"
#include "xJPEG_OptUtils.h"
#include "xDistortion.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================

//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// RDOQ
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint64 xAdvancedEncoder::xCalcDistBLK(const int16* CoeffsQuantScan, const int16* CoeffsTransOrgScan, const uint16* SamplesOrg, const xQuantizer& Quantizer)
{
  if     (m_BlkOptDistCalcMode == eCalkMd::Exact ) { return xCalkExactDistBLK(CoeffsQuantScan, SamplesOrg        , Quantizer); }
  else if(m_BlkOptDistCalcMode == eCalkMd::Approx) { return xCalkApprxDistBLK(CoeffsQuantScan, CoeffsTransOrgScan, Quantizer); }
  else                                             { assert(0); return 0; }
}
uint64 xAdvancedEncoder::xCalkExactDistBLK(const int16* CoeffsQuantScan, const uint16* SamplesOrg, const xQuantizer& Quantizer)
{
  PMBB_ALIGN_JPEG_BLK int16  TmpQuantCoeffs[c_BA];
  PMBB_ALIGN_JPEG_BLK int16  TmpTransCoeffs[c_BA];
  PMBB_ALIGN_JPEG_BLK uint16 TmpSamples    [c_BA];

  xScan::InvScan(TmpQuantCoeffs, CoeffsQuantScan);
  Quantizer.InvScale(TmpTransCoeffs, TmpQuantCoeffs);
  TmpTransCoeffs[0] += xTransformConstants::c_InvDcCorr; //DC correction - JPEG requires 128 to be subtracted from every input sample - could be done be DC -= 
  xTransform::InvTransformDCT_8x8(TmpSamples, TmpTransCoeffs);
  uint64 SSD = xDistortion::CalcSSD(SamplesOrg, TmpSamples, c_BS, c_BS, c_BS, c_BS, 8);
  return SSD;
}
uint64 xAdvancedEncoder::xCalkApprxDistBLK(const int16* CoeffsQuantScan, const int16* CoeffsTransOrg, const xQuantizer& Quantizer)
{
  PMBB_ALIGN_JPEG_BLK int16 RecQuantCoeffs[c_BA];
  PMBB_ALIGN_JPEG_BLK int16 RecTransCoeffs[c_BA];

  xScan::InvScan(RecQuantCoeffs, CoeffsQuantScan);
  Quantizer.InvScale(RecTransCoeffs, RecQuantCoeffs);
  uint64 SSD = xOptUtils::approxSSDfromCoeffs(RecTransCoeffs, CoeffsTransOrg);
  return SSD;
}
void xAdvancedEncoder::xOptQuantPic(int16* OptCoeffsQuantScanV[], const int16* CoeffsQuantScanV[], const int16* CoeffsTransOrgV[], const xPicYUV* Picture)
{
  if     (m_EntropyType == eEntrTp::Huffman   ) { xOptQuantHuffPic(OptCoeffsQuantScanV, CoeffsQuantScanV, CoeffsTransOrgV, Picture); }
  else if(m_EntropyType == eEntrTp::Arithmetic) { xOptQuantAritPic(OptCoeffsQuantScanV, CoeffsQuantScanV, CoeffsTransOrgV, Picture); }
  else                                          { assert(0); }
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG