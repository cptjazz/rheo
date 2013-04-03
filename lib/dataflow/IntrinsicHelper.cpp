#ifndef INTRINSICHELPER_H
#define INTRINSICHELPER_H

#include "IntrinsicHelper.h"
#include <llvm/Intrinsics.h>

bool IntrinsicHelper::getMapping(const Function& f, FunctionTaintMap& mapping) {

  unsigned int intrinsicId = f.getIntrinsicID();

  switch (intrinsicId) {
    case Intrinsic::sqrt:
    case Intrinsic::exp:
    case Intrinsic::cos:
    case Intrinsic::sin:
      mapping.insert(make_pair(0, -1));
      return true;

    case Intrinsic::donothing:
    case Intrinsic::expect:
      // no mapping
      return true;

    default:
      return false;
  }
}

#endif // INTRINSICHELPER_H
