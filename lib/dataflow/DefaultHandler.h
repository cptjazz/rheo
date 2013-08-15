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

    inline void handleInstruction(const Instruction& inst, TaintSet& taintSet) const {
      IF_PROFILING(long t = Helper::getTimestamp());
      const size_t argCount = inst.getNumOperands();
      IF_PROFILING(CTX.logger.profile() << "Get argCount took " << Helper::getTimestampDelta(t) << "µs\n");

      IF_PROFILING(long tl = Helper::getTimestamp());
      for (size_t o_i = 0; o_i < argCount; o_i++) {
        IF_PROFILING(t = Helper::getTimestamp());
        const Value& operand = *inst.getOperand(o_i);
        IF_PROFILING(CTX.logger.profile() << "Get operand took " << Helper::getTimestampDelta(t) << "µs\n");

        if (!CTX.isSupportedInstruction(operand)) {
          CTX.analysisState.stopWithError("Unsupported operand instruction.");
          return;
        }

        if (taintSet.contains(operand)) {
          IF_PROFILING(t = Helper::getTimestamp());
          taintSet.add(inst);
          IF_PROFILING(CTX.logger.profile() << "add to taint set took " << Helper::getTimestampDelta(t) << "µs\n");

          IF_PROFILING(t = Helper::getTimestamp());
          IF_GRAPH(CTX.DOT.addRelation(operand, inst, std::string(Instruction::getOpcodeName(inst.getOpcode()))));
          IF_PROFILING(CTX.logger.profile() << "DOT took " << Helper::getTimestampDelta(t) << "µs\n");

          IF_PROFILING(t = Helper::getTimestamp());
          DEBUG(CTX.logger.debug() << " + Added " << operand << " --> " << inst << "\n");
          IF_PROFILING(CTX.logger.profile() << "DebugLog took " << Helper::getTimestampDelta(t) << "µs\n");
        }
      }
      IF_PROFILING(CTX.logger.profile() << "whole loop took " << Helper::getTimestampDelta(tl) << "µs\n");
    }
};

#endif // DEFAULT_HANDLER_H
