#include "InsertValueHandler.h"
#include "AliasHelper.h"

void InsertValueHandler::handleInstructionInternal(const InsertValueInst& ivInst, TaintSet& taintSet) const {
  const Value& aggregate = *ivInst.getAggregateOperand();
  const Value& val = *ivInst.getInsertedValueOperand();

  DEBUG(CTX.logger.debug() << " Handle INSERTVALUE instruction " << ivInst << "\n");

  if (taintSet.contains(aggregate)) {
    taintSet.add(ivInst);
    IF_GRAPH(CTX.DOT.addRelation(aggregate, ivInst, "insertvalue"));
    //AliasHelper::handleAliasing(CTX, &target, taintSet);
  }

  if (taintSet.contains(val)) {
    taintSet.add(ivInst);
    IF_GRAPH(CTX.DOT.addRelation(val, ivInst, "insertvalue"));
    //AliasHelper::handleAliasing(CTX, &target, taintSet);
  }
}
