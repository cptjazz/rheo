#include "GetElementPtrHandler.h"
#include <sstream>

void GetElementPtrHandler::handleInstructionInternal(const GetElementPtrInst& inst, TaintSet& taintSet) const {
  const Value& op = *inst.getPointerOperand();

  if (taintSet.contains(op)) {
    taintSet.add(inst);
    IF_GRAPH(CTX.DOT.addRelation(op, inst, "indexer"));
    DEBUG(CTX.logger.debug() << " + Added GEP taint: `" << inst << "`\n");
  }

  const size_t numIndices = inst.getNumIndices();
  for (size_t i = 0; i < numIndices; i++) {
    const Value& idx = *inst.getOperand(i + 1);

    if (taintSet.contains(idx)) {
      taintSet.add(inst);
      stringstream reason("");
      DEBUG(reason << "index #" << i);
      IF_GRAPH(CTX.DOT.addRelation(idx, inst, reason.str()));
      DEBUG(CTX.logger.debug() << " ++ Added GEP INDEX: `" << idx << "`\n");
    }
  }
}
