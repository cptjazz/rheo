#ifndef INSTRUCTION_HANDLER_H
#define INSTRUCTION_HANDLER_H

#include "Core.h"

class InstructionHandler {
  unsigned int _opcode;

  protected:
    const DominatorTree& DT;
    PostDominatorTree& PDT;
    GraphExporter& DOT;
    raw_ostream& _stream;

  public:
    InstructionHandler(unsigned int opcode, GraphExporter& dot, const DominatorTree& dt, PostDominatorTree& pdt, raw_ostream& stream) 
      : _opcode(opcode), DT(dt), PDT(pdt), DOT(dot), _stream(stream)
    { }

    virtual ~InstructionHandler() { }

    virtual void handleInstruction(const Instruction& inst, TaintSet& taintSet) const = 0; 

    unsigned int getOpcode() { return _opcode; };
};



template<class T>
class InstructionHandlerTrait : public InstructionHandler {

  public:
    InstructionHandlerTrait(unsigned int opcode, GraphExporter& dot, const DominatorTree& dt, PostDominatorTree& pdt, raw_ostream& stream)
      : InstructionHandler(opcode, dot, dt, pdt, stream) { }

    void handleInstruction(const Instruction& inst, TaintSet& taintSet) const { 
      handleInstructionInternal(cast<T>(inst), taintSet);
    };

  protected:
    virtual void handleInstructionInternal(const T& inst, TaintSet& taintSet) const = 0;
};

#endif // INSTRUCTION_HANDLER_H
