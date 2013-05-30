#include "DefaultHandler.h"

void DefaultHandler::handleInstruction(const Instruction& inst, TaintSet& taintSet) const {
  IF_PROFILING(long t = Helper::getTimestamp());
  const size_t argCount = inst.getNumOperands();
  IF_PROFILING(CTX.logger.profile() << "Get argCount took " << Helper::getTimestampDelta(t) << "µs\n");

  IF_PROFILING(long tl = Helper::getTimestamp());
  for (size_t o_i = 0; o_i < argCount; o_i++) {
    IF_PROFILING(t = Helper::getTimestamp());
    const Value& operand = *inst.getOperand(o_i);
    IF_PROFILING(CTX.logger.profile() << "Get operand took " << Helper::getTimestampDelta(t) << "µs\n");

    if (taintSet.contains(operand)) {
      IF_PROFILING(t = Helper::getTimestamp());
      taintSet.add(inst);
      IF_PROFILING(CTX.logger.profile() << "add to taint set took " << Helper::getTimestampDelta(t) << "µs\n");

      IF_PROFILING(t = Helper::getTimestamp());
      DEBUG(CTX.DOT.addRelation(operand, inst, string(Instruction::getOpcodeName(inst.getOpcode()))));
      IF_PROFILING(CTX.logger.profile() << "DOT took " << Helper::getTimestampDelta(t) << "µs\n");

      IF_PROFILING(t = Helper::getTimestamp());
      DEBUG(CTX.logger.debug() << " + Added " << operand << " --> " << inst << "\n");
      IF_PROFILING(CTX.logger.profile() << "DebugLog took " << Helper::getTimestampDelta(t) << "µs\n");
    }
  }
  IF_PROFILING(CTX.logger.profile() << "whole loop took " << Helper::getTimestampDelta(tl) << "µs\n");
}
