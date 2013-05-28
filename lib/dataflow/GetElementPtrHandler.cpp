#include "GetElementPtrHandler.h"
#include <sstream>

void GetElementPtrHandler::handleInstructionInternal(const GetElementPtrInst& inst, TaintSet& taintSet) const {
  const Value& op = *inst.getPointerOperand();

  if (taintSet.contains(op)) {
    taintSet.add(inst);
    DOT.addRelation(op, inst, "indexer");
    DEBUG_LOG(" + Added GEP taint: `" << inst << "`\n");
  }

  for (size_t i = 0; i < inst.getNumIndices(); i++) {
    const Value& idx = *inst.getOperand(i + 1);

    if (taintSet.contains(idx)) {
      taintSet.add(inst);
      stringstream reason("");
      reason << "index #" << i;
      DOT.addRelation(idx, inst, reason.str());
      DEBUG_LOG(" ++ Added GEP INDEX: `" << idx << "`\n");
    }
  }
}
