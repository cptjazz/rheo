#ifndef FUNCTIONPOINTERHELPER_H
#define FUNCTIONPOINTERHELPER_H

#include "Core.h"
#include "CallHandler.h"
#include "InstructionHandlerContext.h"
#include "llvm/IR/Instructions.h"


class FunctionPointerHelper {
  public:
    typedef SmallPtrSet<const Function*, 16> FunctionSet;

    static void buildMappingWithHeuristic(const CallInst& callInst, ResultSet& taintResults, InstructionHandlerContext& CTX);
    static void buildMappingWithResolve(const CallInst& callInst, InstructionHandlerContext& CTX, FunctionSet& possibleCallees);

  private:
    static void addValueIfFunction(InstructionHandlerContext& CTX, FunctionSet& possibleCallees, const Value* v);
};

#endif // FUNCTIONPOINTERHELPER_H
