#ifndef ALIAS_HELPER_H
#define ALIAS_HELPER_H

#include "Core.h"
#include "TaintSet.h"
#include "InstructionHandlerContext.h"
#include "llvm/Instructions.h"

class AliasHelper {
  public:
  static void handleAliasing(InstructionHandlerContext& CTX, const Instruction& target, TaintSet& taintSet) {
    if (isa<GetElementPtrInst>(target)) {
      const GetElementPtrInst& gep = cast<GetElementPtrInst>(target);
      const Value& ptrOp = *gep.getPointerOperand();
      DEBUG(CTX.logger.debug() << " ++ Added GEP SOURCE:" << ptrOp << "\n");
      DEBUG(CTX.DOT.addRelation(gep, ptrOp, "gep via store"));
      taintSet.add(ptrOp);

      if (isa<Instruction>(ptrOp))
        handleAliasing(CTX, cast<Instruction>(ptrOp), taintSet);
    } else if (isa<LoadInst>(target)) {
      const LoadInst& load = cast<LoadInst>(target);
      const Value& ptrOp = *load.getOperand(0);
      DEBUG(CTX.logger.debug() << " ++ Added LOAD SOURCE:" << ptrOp << "\n");
      DEBUG(CTX.DOT.addRelation(load, ptrOp, "load via store"));
      taintSet.add(ptrOp);

      if (isa<Instruction>(ptrOp))
        handleAliasing(CTX, cast<Instruction>(ptrOp), taintSet);
    } else if(isa<PHINode>(target)) {
      const PHINode& phi = cast<PHINode>(target);
      const size_t preds = phi.getNumIncomingValues();

      for (size_t j = 0; j < preds; j++) {
        const Value& val = *phi.getIncomingValue(j);

        DEBUG(CTX.logger.debug() << " ++ Added PHI SOURCE:" << val << "\n");
        DEBUG(CTX.DOT.addRelation(phi, val, "phi via store"));
        taintSet.add(val);
      }
    }
  }
};

#endif // ALIAS_HELPER_H
