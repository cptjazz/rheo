#ifndef INSTRUCTION_HANDLER_H
#define INSTRUCTION_HANDLER_H

#include <string>
#include "Core.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "GraphExporter.h"
#include "BlockHelper.h"
#include "Helper.h"
#include "InstructionHandlerContext.h"

class InstructionHandler {
  private:
    const unsigned int _opcode;

  public:
    InstructionHandlerContext& CTX;

    InstructionHandler(unsigned int opcode, InstructionHandlerContext& ctx)
      : _opcode(opcode), CTX(ctx)
    { }

    virtual ~InstructionHandler() { }

    virtual void handleInstruction(const Instruction& inst, TaintSet& taintSet) const = 0; 

    inline unsigned int getOpcode() { return _opcode; }
};


template<class T>
class InstructionHandlerTrait : public InstructionHandler {

  public:
    InstructionHandlerTrait(unsigned int opcode, InstructionHandlerContext& ctx)
      : InstructionHandler(opcode, ctx) { }

    inline void handleInstruction(const Instruction& inst, TaintSet& taintSet) const {
      IF_PROFILING(long t = Helper::getTimestamp());
      handleInstructionInternal(cast<T>(inst), taintSet);
      IF_PROFILING(CTX.logger.profile() << "handleInstructionInternal took: " << Helper::getTimestampDelta(t) << "µs\n");
    }

  protected:
    virtual void handleInstructionInternal(const T& inst, TaintSet& taintSet) const = 0;
};


class UnsupportedInstructionHandlerTrait : public InstructionHandlerTrait<Instruction> {
    string _msg;

  public:
    UnsupportedInstructionHandlerTrait(unsigned int opcode, string msg, InstructionHandlerContext& ctx)
        : InstructionHandlerTrait<Instruction>(opcode, ctx), _msg(msg) { }

    inline void handleInstructionInternal(const Instruction& inst, TaintSet& taintSet) const {
      CTX.analysisState.stopWithError(_msg);
    }
};

#endif // INSTRUCTION_HANDLER_H
