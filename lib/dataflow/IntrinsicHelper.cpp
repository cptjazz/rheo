#ifndef INTRINSICHELPER_H
#define INTRINSICHELPER_H

#include "IntrinsicHelper.h"
#include <llvm/Intrinsics.h>

bool IntrinsicHelper::getMapping(const Function& f, FunctionTaintMap& mapping) {

  unsigned int intrinsicId = f.getIntrinsicID();

  switch (intrinsicId) {
    case Intrinsic::sqrt:
    case Intrinsic::exp:
    case Intrinsic::exp2:
    case Intrinsic::cos:
    case Intrinsic::sin:
    case Intrinsic::log:
    case Intrinsic::log10:
    case Intrinsic::log2:
    case Intrinsic::fabs:
    case Intrinsic::floor:
    //case Intrinsic::ceil:
    //case Intrinsic::trunc:
    //case Intrinsic::rint:
    //case Intrinsic::nearbyint:
      // 0 => $_retval
      mapping.insert(make_pair(0, -1));
      return true;

    case Intrinsic::pow:
    case Intrinsic::powi:
      // 0 => $_retval, 1 => $_retval
      mapping.insert(make_pair(0, -1));
      mapping.insert(make_pair(1, -1));
      return true;

    case Intrinsic::memcpy:
      // 1 (src) => 0 (dst), 2 (len) => 0 (dst)
      // due to overlapping src can also be overwritten: 0 => 1
      mapping.insert(make_pair(1, 0));
      mapping.insert(make_pair(2, 0));
      mapping.insert(make_pair(0, 1));
      return true;

    case Intrinsic::memmove:
    case Intrinsic::memset:
      // 1 (src) => 0 (dst), 2 (len) => 0 (dst)
      // memmove: overlapping is handled correctly
      mapping.insert(make_pair(1, 0));
      mapping.insert(make_pair(2, 0));
      return true;

    // fused multiply-add
    case Intrinsic::fma:
      // 0 => $_retval, 1 => $_retval, 2 => $_retval
      mapping.insert(make_pair(0, -1));
      mapping.insert(make_pair(1, -1));
      mapping.insert(make_pair(2, -1));
      return true;

    case Intrinsic::donothing:
    case Intrinsic::expect:
    case Intrinsic::pcmarker:
    case Intrinsic::prefetch:
    case Intrinsic::lifetime_start:
    case Intrinsic::lifetime_end:
    case Intrinsic::invariant_start:
    case Intrinsic::invariant_end:
    case Intrinsic::vastart:
    case Intrinsic::vacopy:
    case Intrinsic::vaend:
    case Intrinsic::flt_rounds:
      // no mapping required
      return true;

    default:
      // no mapping for intrinsic provided -> raise error
      return false;
  }
}

#endif // INTRINSICHELPER_H
