#ifndef INSTRUCTION_HANDLER_DISPATCHER_H
#define INSTRUCTION_HANDLER_DISPATCHER_H

class InstructionHandlerDispatcher {

  map<unsigned int, InstructionHandler*> mapping;
  InstructionHandler* _defaultHandler;
  const DominatorTree& DT;
  PostDominatorTree& PDT;
  GraphExporter& DOT;
  raw_ostream& _stream;

  struct DeleteFunctor {
    template<class T>
    void operator()(const T& object) const {
      delete object.second;
    }
  };

  public:
    InstructionHandlerDispatcher(GraphExporter& dot, const DominatorTree& dt, PostDominatorTree& pdt, raw_ostream& stream) 
      : DT(dt), PDT(pdt), DOT(dot), _stream(stream) 
    { }

    template<class T>
    void registerHandler() {
      T* handler = new T(DOT, DT, PDT, _stream);
      mapping.insert(make_pair(handler->getOpcode(), handler));
    }

    template<class T>
    void registerDefaultHandler() {
      _defaultHandler = new T(DOT, DT, PDT, _stream);
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
