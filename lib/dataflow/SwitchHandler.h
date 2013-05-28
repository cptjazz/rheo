#ifndef SWITCH_HANDLER_H
#define SWITCH_HANDLER_H

#include "InstructionHandler.h"

/**
 * SWITCH is handled the following way:
 *
 * If the condition is tainted, each case (or default) is tainted
 * due to nesting.
 */
class SwitchHandler : public InstructionHandlerTrait<SwitchInst> {

  public:
    SwitchHandler(InstructionHandlerContext& ctx)
      : InstructionHandlerTrait(Instruction::Switch, ctx) { }

    void handleInstructionInternal(const SwitchInst& inst, TaintSet& taintSet) const;
};

#endif // SWITCH_HANDLER_H
