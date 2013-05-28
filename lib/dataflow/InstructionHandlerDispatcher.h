#ifndef INSTRUCTION_HANDLER_DISPATCHER_H
#define INSTRUCTION_HANDLER_DISPATCHER_H

#include "InstructionHandler.h"

class InstructionHandlerDispatcher {

  map<unsigned int, InstructionHandler*> mapping;
  InstructionHandler* _defaultHandler;
  InstructionHandlerContext& _context;

  struct DeleteFunctor {
    template<class T>
    void operator()(const T& object) const {
      delete object.second;
    }
  };

  public:
    InstructionHandlerDispatcher(GraphExporter& dot, const DominatorTree& dt, PostDominatorTree& pdt, raw_ostream& stream,
                                 deque<const BasicBlock*>& worklist)
        : _context(*new InstructionHandlerContext(dot, dt, pdt, stream, worklist))
    { }

    template<class T>
    void registerHandler() {
      T* handler = new T(_context);
      mapping.insert(make_pair(handler->getOpcode(), handler));
    }

    template<class T>
    void registerDefaultHandler() {
      _defaultHandler = new T(_context);
    }

    void dispatch(const Instruction& instruction, TaintSet& taintSet) {
      InstructionHandler* handler = mapping[instruction.getOpcode()];

      if (handler != NULL)
        handler->handleInstruction(instruction, taintSet);
      else
        _defaultHandler->handleInstruction(instruction, taintSet);

    }

    ~InstructionHandlerDispatcher() {
      for_each(mapping.begin(), mapping.end(), DeleteFunctor());
      mapping.clear();
    }
};

#endif // INSTRUCTION_HANDLER_DISPATCHER_H
