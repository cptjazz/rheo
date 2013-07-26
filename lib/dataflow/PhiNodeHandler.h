#ifndef PHINODE_HANDLER_H
#define PHINODE_HANDLER_H

#include "InstructionHandler.h"

class PhiNodeHandler : public InstructionHandlerTrait<PHINode> {

  public:
    PhiNodeHandler(InstructionHandlerContext& ctx)
      : InstructionHandlerTrait(Instruction::PHI, ctx) { }

    void handleInstructionInternal(const PHINode& inst, TaintSet& taintSet) const {
      const size_t incomingCount = inst.getNumIncomingValues();

      for (size_t j = 0; j < incomingCount; ++j) {
        BasicBlock& incomingBlock = *inst.getIncomingBlock(j);
        Value& incomingValue = *inst.getIncomingValue(j);

        // We need to handle block-tainting here, because
        // with PHI nodes, the effective assignment is no
        // longer in the previous block, but in the PHI.
        // If you assign constants in the block and the block
        // is tainted by an if, we would not see this taint,
        // because our logic would say: "is <const 7> tainted. no".
        // Without phi we would have an explicit (block tainted) assignment
        // or store in the block.
        if (taintSet.contains(incomingBlock)) {
          taintSet.add(Taint::make_infered(inst));
          DEBUG(CTX.logger.debug() << " + Added PHI from block" << incomingBlock << "\n");
          DEBUG(CTX.DOT.addRelation(incomingBlock, inst, "block-taint"));
        } else if (taintSet.contains(incomingValue)) {
          taintSet.add(Taint::make_infered(inst));
          DEBUG(CTX.logger.debug() << " + Added PHI from value" << incomingValue << "\n");
          DEBUG(CTX.DOT.addRelation(incomingValue, inst, "phi-value"));
        }
      }
    }
};

#endif // PHINODE_HANDLER_H
