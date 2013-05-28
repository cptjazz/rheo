#ifndef DEFAULT_HANDLER_H
#define DEFAULT_HANDLER_H

#include "InstructionHandler.h"

/**
 * An arbitrary Instruction is handled the following way:
 *
 * If one of the operands is tainted, the taitn is transfered to
 * the assignment target.
 */
class DefaultHandler : public InstructionHandler {

  public:
    DefaultHandler(GraphExporter& dot, const DominatorTree& dt, PostDominatorTree& pdt, raw_ostream& stream)
      : InstructionHandler(0, dot, dt, pdt, stream) { }

    void handleInstruction(const Instruction& inst, TaintSet& taintSet) const {
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
};

#endif // DEFAULT_HANDLER_H
