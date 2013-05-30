#include "StoreHandler.h"

void StoreHandler::handleInstructionInternal(const StoreInst& storeInst, TaintSet& taintSet) const {
  const Value& source = *storeInst.getOperand(0);
  const Value& target = *storeInst.getOperand(1);

  DEBUG(CTX.logger.debug() << " Handle STORE instruction " << storeInst << "\n");
  if (taintSet.contains(source) || taintSet.contains(storeInst)) {
    taintSet.add(target);
    taintSet.add(storeInst);

    DEBUG(CTX.DOT.addRelation(source, target, "store"));
    DEBUG(CTX.logger.debug() << " + Added STORE taint: " << source << " --> " << target << "\n");
    if (isa<GetElementPtrInst>(target) || isa<LoadInst>(target)) {
      const Instruction& inst = cast<Instruction>(target);
      recursivelyAddAllGepsAndLoads(inst, taintSet);
    }

  } else if (taintSet.contains(target)) {
    // Only do removal if value is really in set
    taintSet.remove(target);
    DEBUG(CTX.logger.debug() << " - Removed STORE taint due to non-tainted overwrite: " << source << " --> " << target << "\n");
    DEBUG(CTX.DOT.addRelation(source, target, "non-taint overwrite"));
  }
}


void StoreHandler::recursivelyAddAllGepsAndLoads(const Instruction& target, TaintSet& taintSet) const {
  if (isa<GetElementPtrInst>(target)) {
    const GetElementPtrInst& gep = cast<GetElementPtrInst>(target);
    const Value& ptrOp = *gep.getPointerOperand();
    DEBUG(CTX.logger.debug() << " ++ Added GEP SOURCE:" << ptrOp << "\n");
    taintSet.add(ptrOp);

    DEBUG(CTX.DOT.addRelation(gep, ptrOp, "gep via store"));

    if (isa<Instruction>(ptrOp))
      recursivelyAddAllGepsAndLoads(cast<Instruction>(ptrOp), taintSet);
  } else if (isa<LoadInst>(target)) {
    const LoadInst& load = cast<LoadInst>(target);
    const Value& ptrOp = *load.getOperand(0);
    DEBUG(CTX.logger.debug() << " ++ Added LOAD SOURCE:" << ptrOp << "\n");
    taintSet.add(ptrOp);

    DEBUG(CTX.DOT.addRelation(load, ptrOp, "load via store"));

    if (isa<Instruction>(ptrOp))
      recursivelyAddAllGepsAndLoads(cast<Instruction>(ptrOp), taintSet);
  }
}
