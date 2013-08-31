#ifndef HEURISTICHELPER_H
#define HEURISTICHELPER_H

#include "Core.h"
#include "InstructionHandlerContext.h"
#include "llvm/IR/Instructions.h"
#include <vector>

struct HeuristicHelper {

  /**
   * For calls to function pointers or exterals we use conservative heuristic:
   * 1) Every parameter taints the return value
   * 2) Every parameter taints all out pointers
   */
  static void buildMapping(const CallInst& callInst, ResultSet& taintResults, InstructionHandlerContext& CTX) {
    std::vector<const Value*> arguments;
    const size_t argCount = callInst.getNumArgOperands();

    bool isExternal = callInst.getCalledFunction() != NULL;

    arguments.reserve(argCount + 30);
    for (size_t j = 0; j < argCount; ++j) {
      arguments.push_back(callInst.getArgOperand(j));
    }

    for (Module::const_global_iterator g_i = CTX.M.global_begin(), g_e = CTX.M.global_end(); g_i != g_e; ++g_i) {
      const GlobalVariable& g = *g_i;

      // Skip constants (eg. string literals)
      if (g.isConstant())
        continue;

      if (isExternal && !CTX.EXCL.includesFunction(&CTX.F) && (g.hasInternalLinkage() || g.hasPrivateLinkage())) {
        continue;
      }

      arguments.push_back(&g);
    }

    const size_t argumentsCount = arguments.size();

    for (size_t i = 0; i < argumentsCount; i++) {
      const Value& source = *arguments.at(i);
      int sourcePos = isa<GlobalVariable>(source) ? GLOBAL_POSITION : i;

      // Every argument taints the return value
      taintResults.insert(std::make_pair(&source, &callInst));
      DEBUG(CTX.logger.debug() << "Heuristic: inserting mapping " << sourcePos << " => -1\n");

      // Every argument taints other pointer arguments (out-arguments)
      for (size_t j = 0; j < argumentsCount; j++) {
        const Value& sink = *arguments.at(j);
        int sinkPos = isa<GlobalVariable>(sink) ? GLOBAL_POSITION : j;

        if (sink.getType()->isPointerTy() || isa<GlobalVariable>(sink)) {
          DEBUG(CTX.logger.debug() << "Heuristic: type" << *sink.getType() << "\n");
          // GlobalVariable is a subclass of `Constant`, so we have to check if
          // the Constant is not a GlobalVariable to really be a constant.
          // Constant (now used as adjective, not the class name) globals are 
          // already filtered above, so this should work.
          if (isa<Constant>(sink) && !isa<GlobalVariable>(sink))
            continue;

          DEBUG(CTX.logger.debug() << "Heuristic: inserting mapping " << sourcePos << " => " << sinkPos << "\n");
          taintResults.insert(std::make_pair(&source, &sink));
        }
      }
    }

    CTX.mappingCache.insert(std::make_pair(&callInst, taintResults));
  }
};

#endif // HEURISTICHELPER_H
