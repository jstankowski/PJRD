/*
    SPDX-FileCopyrightText: 2020-2026 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/
#include "xCommonDefJPEG.h"
#include "xString.h"

namespace PMBB_NAMESPACE::JPEG {

//=============================================================================================================================================================================

eImpl xStrToImpl(const std::string& Impl)
{
  std::string ImplL = xString::toLower(Impl);
  return ImplL == "simple"   ? eImpl::Simple   :
         ImplL == "advanded" ? eImpl::Advanced :
                               eImpl::INVALID  ;
}
std::string xImplToStr(eImpl Impl)
{
  return Impl == eImpl::Simple   ? "Simple"   :
         Impl == eImpl::Advanced ? "Advanced" :
                                   "INVALID"  ;
}
eQTLa xStrToQTLa(const std::string& QTLa)
{
  std::string QTLaL = xString::toLower(QTLa);
  return QTLaL == "default"  ? eQTLa::Default  :
         QTLaL == "flat"     ? eQTLa::Flat     :
         QTLaL == "semiflat" ? eQTLa::SemiFlat :
                               eQTLa::INVALID  ;
}
std::string xQTLaToStr(eQTLa QTLa)
{
  return QTLa == eQTLa::Default  ? "Default"  :
         QTLa == eQTLa::Flat     ? "Flat"     :
         QTLa == eQTLa::SemiFlat ? "SemiFlat" :
                                   "INVALID"  ;
}
eCalkMd xStrToCalkMd(const std::string& CalkMd)
{
  std::string CalkMdL = xString::toLower(CalkMd);
  return CalkMdL == "exact"  ? eCalkMd::Exact  :
         CalkMdL == "approx" ? eCalkMd::Approx :
                               eCalkMd::INVALID;
}
std::string xCalkMdToStr(eCalkMd CalkMd)
{
  switch(CalkMd)
  {
  case eCalkMd::Exact : return "Exact" ; 
  case eCalkMd::Approx: return "Approx";
  default: return "INVALID";
  }
}
eEntrTp xStrToEntrTp(const std::string& EntrTp)
{
  std::string EntrTpL = xString::toLower(EntrTp);
  return EntrTpL == "huffman"    ? eEntrTp::Huffman    :
         EntrTpL == "arithmetic" ? eEntrTp::Arithmetic :
                                   eEntrTp::INVALID    ;
}
std::string xEntrTpToStr(eEntrTp EntrTp)
{
  switch(EntrTp)
  {
    case eEntrTp::Huffman   : return "Huffman"   ;
    case eEntrTp::Arithmetic: return "Arithmetic";
    default                 : return "INVALID"   ;
  }
}
eLmbd xStrToLmbd(const std::string& Lmbd)
{
  std::string LmbdL = xString::toLower(Lmbd);
  return LmbdL == "exhaustive3" ? eLmbd::Exhaustive3 :
         LmbdL == "exhaustive5" ? eLmbd::Exhaustive5 :
         LmbdL == "approxexact" ? eLmbd::ApproxExact :
         LmbdL == "approxfast"  ? eLmbd::ApproxFast  :
                                  eLmbd::INVALID     ;
}
std::string xLmbdToStr(eLmbd Lmbd)
{
  switch(Lmbd)
  {
  case eLmbd::Exhaustive3 : return "Exhaustive3"; 
  case eLmbd::Exhaustive5 : return "Exhaustive5"; 
  case eLmbd::ApproxExact : return "ApproxExact";
  case eLmbd::ApproxFast  : return "ApproxFast" ;
  default: return "INVALID";
  }
}
eQntOpt xStrToQntOpt(const std::string& QntOpt)
{
  std::string QntOptL = xString::toLower(QntOpt);
  return QntOptL == "greedy"    ? eQntOpt::Greedy    :
                                  eQntOpt::INVALID   ;
}
std::string xQntOptToStr(eQntOpt QntOpt)
{
  switch(QntOpt)
  {
  case eQntOpt::Greedy    : return "Greedy"    ;
  default                 : return "INVALID"   ;
  }
}

//=============================================================================================================================================================================

} //end of namespace PMBB::JPEG