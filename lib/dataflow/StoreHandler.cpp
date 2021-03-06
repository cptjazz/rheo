#include "StoreHandler.h"
#include "AliasHelper.h"

void StoreHandler::handleInstructionInternal(const StoreInst& storeInst, TaintSet& taintSet) const {
  const Value& source = *storeInst.getOperand(0);
  const Value& target = *storeInst.getOperand(1);

  DEBUG(CTX.logger.debug() << " Handle STORE instruction " << storeInst << "\n");
  if (taintSet.contains(source) || taintSet.contains(storeInst)) {
    taintSet.add(target);

    IF_GRAPH(CTX.DOT.addRelation(source, target, "store"));
    DEBUG(CTX.logger.debug() << " + Added STORE taint: " << source << " --> " << target << "\n");

    AliasHelper::handleAliasing(CTX, &target, taintSet);
  } else if (taintSet.contains(target)) {
    // Only do removal if value is really in set
    DEBUG(CTX.logger.debug() << "Set size before: " << taintSet.size() << "\n");
    taintSet.remove(target);
    DEBUG(CTX.logger.debug() << "Set size after: " << taintSet.size() << "\n");

    DEBUG(CTX.logger.debug() << " - Removed STORE taint due to non-tainted overwrite: " << source << " --> " << target << "\n");
    IF_GRAPH(CTX.DOT.addRelation(source, target, "non-taint overwrite"));
  }
}


