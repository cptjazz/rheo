#include "AliasHelper.h"
#include "llvm/IR/Instructions.h"

void AliasHelper::handleAliasingInternal(InstructionHandlerContext& CTX, const Value* target, TaintSet& taintSet, ValueSet& worklist) {
  if (!target)
    return;

  if (worklist.count(target))
    return;

  worklist.insert(target);

  DEBUG(CTX.logger.debug() << "handle alias: " << *target << "\n");

  //if (!target->getType()->isPointerTy())
  //return;

  if (!CTX.isSupportedInstruction(*target)) {
    CTX.analysisState.stopWithError(std::string("Unsupported instruction while resolving aliases: ")
        + Instruction::getOpcodeName(cast<Instruction>(target)->getOpcode()));
    return;
  }

  if (const GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(target)) {
    const Value* ptrOp = gep->getPointerOperand();
    DEBUG(CTX.logger.debug() << " ++ Added GEP SOURCE:" << *ptrOp << "\n");
    IF_GRAPH(CTX.DOT.addRelation(*gep, *ptrOp, "gep-alias"));
    taintSet.add(*ptrOp);

    //if (const Instruction* inst = dyn_cast<Instruction>(ptrOp))
    //handleAliasing(CTX, inst, taintSet);
    handleAliasingInternal(CTX, ptrOp, taintSet, worklist);

  } else if (const LoadInst* load = dyn_cast<LoadInst>(target)) {
    const Value* ptrOp = load->getOperand(0);
    DEBUG(CTX.logger.debug() << " ++ Added LOAD SOURCE:" << *ptrOp << "\n");
    IF_GRAPH(CTX.DOT.addRelation(*load, *ptrOp, "load-alias"));
    taintSet.add(*ptrOp);

    //if (const Instruction* inst = dyn_cast<Instruction>(ptrOp))
    //handleAliasing(CTX, inst, taintSet);
    handleAliasingInternal(CTX, ptrOp, taintSet, worklist);

  } else if (const PHINode* phi = dyn_cast<PHINode>(target)) {
    const size_t preds = phi->getNumIncomingValues();

    for (size_t j = 0; j < preds; ++j) {
      const Value& val = *phi->getIncomingValue(j);

      DEBUG(CTX.logger.debug() << " ++ Added PHI SOURCE:" << val << "\n");
      IF_GRAPH(CTX.DOT.addRelation(*phi, val, "phi-alias"));
      taintSet.add(val);

      handleAliasingInternal(CTX, &val, taintSet, worklist);
    }

  } else if (const CastInst* inst = dyn_cast<CastInst>(target)) {
    const Value& val = *inst->getOperand(0);

    DEBUG(CTX.logger.debug() << " ++ Added CAST SOURCE:" << val << "\n");
    IF_GRAPH(CTX.DOT.addRelation(*target, val, "cast-alias"));
    taintSet.add(val);
    handleAliasingInternal(CTX, &val, taintSet, worklist);

  } else if (CTX.STH.hasAlias(target)) {
    const ValueSet& set = CTX.STH.getAliases(target);
    for (ValueSet::iterator i = set.begin(), e = set.end(); i != e; ++i) {
      const Value* v = *i;
      DEBUG(CTX.logger.debug() << " ++ Added SPECIAL ALIAS:" << *v << "\n");
      IF_GRAPH(CTX.DOT.addRelation(*target, *v, "special-alias"));
      taintSet.add(*v);
      handleAliasingInternal(CTX, v, taintSet, worklist);
    }
  }
}
