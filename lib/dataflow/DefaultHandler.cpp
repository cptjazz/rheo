#include "DefaultHandler.h"

void DefaultHandler::handleInstruction(const Instruction& inst, TaintSet& taintSet) const {
  const size_t argCount = inst.getNumOperands();

  for (size_t o_i = 0; o_i < argCount; o_i++) {
    const Value& operand = *inst.getOperand(o_i);
    if (taintSet.contains(operand)) {
      taintSet.add(inst);
      DOT.addRelation(operand, inst, string(Instruction::getOpcodeName(inst.getOpcode())));

      DEBUG_LOG(" + Added " << operand << " --> " << inst << "\n");
    }
  }
}
