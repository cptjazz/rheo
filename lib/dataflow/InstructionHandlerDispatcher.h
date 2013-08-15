#ifndef INSTRUCTION_HANDLER_DISPATCHER_H
#define INSTRUCTION_HANDLER_DISPATCHER_H

#include "InstructionHandler.h"
#include "Core.h"
#include "AnalysisState.h"
#include "Helper.h"
#include "SupportedInstructionFunctor.h"


class InstructionHandlerDispatcher {
  private:
    std::map<unsigned int, InstructionHandler*> mapping;
    InstructionHandler* _defaultHandler;
    InstructionHandlerContext& _context;
    SupportedInstructionFunctor unsupportedFunctor;

    struct DeleteFunctor {
      template<class T>
        void operator()(const T& object) const {
          delete object.second;
        }
    };

    void initialize();

  public:
    InstructionHandlerDispatcher(InstructionHandlerContext& ctx) : _context(ctx) {
      _context.registerIsSupportedInstructionCallback(unsupportedFunctor);
    }

    ~InstructionHandlerDispatcher() {
      for_each(mapping.begin(), mapping.end(), DeleteFunctor());
      mapping.clear();
    }

    template<class T>
    T* registerHandler() {
      T* handler = new T(_context);
      mapping.insert(std::make_pair(handler->getOpcode(), handler));
      return handler;
    }

    template<class T>
    void registerHandlerForUnsupportedInstruction() {
      T* handler = registerHandler<T>();
      unsupportedFunctor.registerUnsupportedInstruction(handler->getOpcode());
    }

    template<class T>
    void registerDefaultHandler() {
      _defaultHandler = new T(_context);
    }

    inline void dispatch(const Instruction& instruction, TaintSet& taintSet) {
      IF_PROFILING(long t = Helper::getTimestamp());
      InstructionHandler* handler = mapping[instruction.getOpcode()];

      if (handler != NULL) {
        DEBUG(_context.logger.output() << "USING HANDLER FOR " << instruction.getOpcode() << "\n");
        handler->handleInstruction(instruction, taintSet);
      } else {
        DEBUG(_context.logger.output() << "USING DEFAULT HANDLER\n");
        _defaultHandler->handleInstruction(instruction, taintSet);
      }

      IF_PROFILING(_context.logger.profile() << "dispatch took " << Helper::getTimestampDelta(t) << "µs\n");
    }
};

#endif // INSTRUCTION_HANDLER_DISPATCHER_H
