#ifndef INSTRUCTION_HANDLER_DISPATCHER_H
#define INSTRUCTION_HANDLER_DISPATCHER_H

#include "InstructionHandler.h"
#include "Core.h"
#include "InstructionHandler.h"
#include "AnalysisState.h"
#include "Helper.h"


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
  InstructionHandlerDispatcher(InstructionHandlerContext& ctx) : _context(ctx)
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

    inline void dispatch(const Instruction& instruction, TaintSet& taintSet) {
      IF_PROFILING(long t = Helper::getTimestamp());
      InstructionHandler* handler = mapping[instruction.getOpcode()];

      if (handler != NULL)
        handler->handleInstruction(instruction, taintSet);
      else
        _defaultHandler->handleInstruction(instruction, taintSet);
      IF_PROFILING(_context.logger.profile() << "dispatch took " << Helper::getTimestampDelta(t) << "µs\n");
    }

    ~InstructionHandlerDispatcher() {
      for_each(mapping.begin(), mapping.end(), DeleteFunctor());
      mapping.clear();
    }
};

#endif // INSTRUCTION_HANDLER_DISPATCHER_H
