#include "StoreHandler.h"
#include "AliasHelper.h"

void StoreHandler::handleInstructionInternal(const StoreInst& storeInst, TaintSet& taintSet) const {
  const Value& source = *storeInst.getOperand(0);
  const Value& target = *storeInst.getOperand(1);

  DEBUG(CTX.logger.debug() << " Handle STORE instruction " << storeInst << "\n");
  if (taintSet.contains(source) || taintSet.contains(storeInst)) {
    taintSet.add(target);

    DEBUG(CTX.DOT.addRelation(source, target, "store"));
    DEBUG(CTX.logger.debug() << " + Added STORE taint: " << source << " --> " << target << "\n");

    if (isa<GetElementPtrInst>(target) || isa<LoadInst>(target) || isa<PHINode>(target)) {
      const Instruction& inst = cast<Instruction>(target);
      AliasHelper::handleAliasing(CTX, inst, taintSet);
    }

  } else if (taintSet.contains(target)) {
    // Only do removal if value is really in set
    taintSet.remove(target);
    DEBUG(CTX.logger.debug() << " - Removed STORE taint due to non-tainted overwrite: " << source << " --> " << target << "\n");
    DEBUG(CTX.DOT.addRelation(source, target, "non-taint overwrite"));
  }
}


