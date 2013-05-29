#ifndef INSTRUCTION_HANDLER_H
#define INSTRUCTION_HANDLER_H

#include "Core.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/raw_ostream.h"
#include "GraphExporter.h"
#include "llvm/Instructions.h"
#include "BlockHelper.h"
#include "InstructionHandlerContext.h"
#include <string>

class InstructionHandler {
  private:
    unsigned int _opcode;

  public:
    InstructionHandlerContext& CTX;

    InstructionHandler(unsigned int opcode, InstructionHandlerContext& ctx)
      : _opcode(opcode), CTX(ctx)
    { }

    virtual ~InstructionHandler() { }

    virtual void handleInstruction(const Instruction& inst, TaintSet& taintSet) const = 0; 

    unsigned int getOpcode() { return _opcode; }
};


template<class T>
class InstructionHandlerTrait : public InstructionHandler {

  public:
    InstructionHandlerTrait(unsigned int opcode, InstructionHandlerContext& ctx)
      : InstructionHandler(opcode, ctx) { }

    void handleInstruction(const Instruction& inst, TaintSet& taintSet) const { 
      handleInstructionInternal(cast<T>(inst), taintSet);
    }

  protected:
    virtual void handleInstructionInternal(const T& inst, TaintSet& taintSet) const = 0;
};


template<class T>
class UnsupportedInstructionHandlerTrait : public InstructionHandlerTrait<T> {
    string _msg;

  public:
    UnsupportedInstructionHandlerTrait(string msg, InstructionHandlerContext& ctx)
        : InstructionHandlerTrait<T>(0, ctx), _msg(msg) { }

    void handleInstructionInternal(const T& inst, TaintSet& taintSet) const {
        this->CTX.analysisState.stopWithError(_msg);
    }
};

#endif // INSTRUCTION_HANDLER_H
