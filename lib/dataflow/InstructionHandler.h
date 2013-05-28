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


class InstructionHandler {
  unsigned int _opcode;

  protected:
    const DominatorTree& DT;
    PostDominatorTree& PDT;
    GraphExporter& DOT;
    const BlockHelper& BH;
    InstructionHandlerContext& CTX;
    raw_ostream& _stream;

  public:
    InstructionHandler(unsigned int opcode, InstructionHandlerContext& ctx)
      : _opcode(opcode), DT(ctx.DT), PDT(ctx.PDT), DOT(ctx.DOT),
        BH(*new BlockHelper(ctx.DT, ctx.PDT, ctx.DOT, ctx.stream)), CTX(ctx), _stream(ctx.stream)
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

#endif // INSTRUCTION_HANDLER_H
