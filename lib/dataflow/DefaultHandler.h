#ifndef DEFAULT_HANDLER_H
#define DEFAULT_HANDLER_H

#include "InstructionHandler.h"

/**
 * An arbitrary Instruction is handled the following way:
 *
 * If one of the operands is tainted, the taint is transfered to
 * the assignment target.
 */
class DefaultHandler : public InstructionHandler {

  public:
    DefaultHandler(InstructionHandlerContext& ctx)
      : InstructionHandler(0, ctx) { }

    void handleInstruction(const Instruction& inst, TaintSet& taintSet) const;
};

#endif // DEFAULT_HANDLER_H
