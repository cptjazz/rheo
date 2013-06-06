#ifndef INSTRUCTION_HANDLER_H
#define INSTRUCTION_HANDLER_H

#include "Core.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/raw_ostream.h"
#include "GraphExporter.h"
#include "llvm/Instructions.h"
#include "BlockHelper.h"
#include "Helper.h"
#include "InstructionHandlerContext.h"
#include <string>

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

    void handleInstructionInternal(const Instruction& inst, TaintSet& taintSet) const {
      this->CTX.analysisState.stopWithError(_msg);
    }
};

#endif // INSTRUCTION_HANDLER_H
