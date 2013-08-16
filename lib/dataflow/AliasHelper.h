#ifndef ALIAS_HELPER_H
#define ALIAS_HELPER_H

#include "Core.h"
#include "TaintSet.h"
#include "InstructionHandlerContext.h"

class AliasHelper {
  private:
    static void handleAliasingInternal(InstructionHandlerContext& CTX, const Value* target, TaintSet& taintSet, ValueSet& worklist);

  public:
    inline static void handleAliasing(InstructionHandlerContext& CTX, const Value* target, TaintSet& taintSet) {
      ValueSet worklist;
      handleAliasingInternal(CTX, target, taintSet, worklist);
    }
};

#endif // ALIAS_HELPER_H
