#ifndef ALIAS_HELPER_H
#define ALIAS_HELPER_H

#include "Core.h"
#include "TaintSet.h"
#include "InstructionHandlerContext.h"
#include "llvm/IR/Instructions.h"

class AliasHelper {
  public:
  static void handleAliasing(InstructionHandlerContext& CTX, const Value* target, TaintSet& taintSet) {
    if (!target->getType()->isPointerTy())
      return;

    if (!CTX.isSupportedInstruction(*target)) {
      CTX.analysisState.stopWithError(string("Unsupported instruction while resolving aliases: ") 
          + Instruction::getOpcodeName(cast<Instruction>(target)->getOpcode()));
      return;
    }

    if (const GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(target)) {
      const Value* ptrOp = gep->getPointerOperand();
      DEBUG(CTX.logger.debug() << " ++ Added GEP SOURCE:" << *ptrOp << "\n");
      DEBUG(CTX.DOT.addRelation(*gep, *ptrOp, "gep-alias"));
      taintSet.add(*ptrOp);

      if (const Instruction* inst = dyn_cast<Instruction>(ptrOp))
        handleAliasing(CTX, inst, taintSet);

    } else if (const LoadInst* load = dyn_cast<LoadInst>(target)) {
      const Value* ptrOp = load->getOperand(0);
      DEBUG(CTX.logger.debug() << " ++ Added LOAD SOURCE:" << *ptrOp << "\n");
      DEBUG(CTX.DOT.addRelation(*load, *ptrOp, "load-alias"));
      taintSet.add(*ptrOp);

      if (const Instruction* inst = dyn_cast<Instruction>(ptrOp))
        handleAliasing(CTX, inst, taintSet);

    } else if(const PHINode* phi = dyn_cast<PHINode>(target)) {
      const size_t preds = phi->getNumIncomingValues();

      for (size_t j = 0; j < preds; ++j) {
        const Value& val = *phi->getIncomingValue(j);

        DEBUG(CTX.logger.debug() << " ++ Added PHI SOURCE:" << val << "\n");
        DEBUG(CTX.DOT.addRelation(*phi, val, "phi-alias"));
        taintSet.add(val);
      }
    } else if(const CastInst* inst = dyn_cast<CastInst>(target)) {
      const Value& val = *inst->getOperand(0);

      DEBUG(CTX.logger.debug() << " ++ Added CAST SOURCE:" << val << "\n");
      DEBUG(CTX.DOT.addRelation(*target, val, "cast-alias"));
      taintSet.add(val);
    }
  }
};

#endif // ALIAS_HELPER_H
