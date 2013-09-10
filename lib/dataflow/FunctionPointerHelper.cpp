#include "FunctionPointerHelper.h"
#include "HeuristicHelper.h"

void FunctionPointerHelper::buildMappingWithHeuristic(const CallInst& callInst, ResultSet& taintResults, InstructionHandlerContext& CTX) {
  HeuristicHelper::buildMapping(callInst, taintResults, CTX);
}

void FunctionPointerHelper::buildMappingWithResolve(const CallInst& callInst, InstructionHandlerContext& CTX, FunctionSet& possibleCallees) {
  // We search for realisations of the pointer and can then 
  // union the call-mappings of all possible callees
  //
  
  bool allFunctionsResolvable = true;

  const Value* delegate = callInst.getCalledValue();
  DEBUG(CTX.logger.debug() << "Resolve function pointers for: " << *delegate << "\n");

  if (const PHINode* phi = dyn_cast<PHINode>(delegate)) {
    for (size_t j = 0; j < phi->getNumIncomingValues(); j++) {
      allFunctionsResolvable &= addValueIfFunction(CTX, possibleCallees, phi->getIncomingValue(j));
    }
  } else if (const SelectInst* select = dyn_cast<SelectInst>(delegate)) {
      allFunctionsResolvable &= addValueIfFunction(CTX, possibleCallees, select->getTrueValue());
      allFunctionsResolvable &= addValueIfFunction(CTX, possibleCallees, select->getFalseValue());
  }

  // If one callee is unresolvable, we need to fall back
  // to heuristic. This will happen if no callees were found.
  if (!allFunctionsResolvable)
    possibleCallees.clear();
}

bool FunctionPointerHelper::addValueIfFunction(InstructionHandlerContext& CTX, FunctionSet& possibleCallees, const Value* v) {
  if (const Function* func = dyn_cast<Function>(v)) {
    possibleCallees.insert(func);
    DEBUG(CTX.logger.debug() << "Added realisation: " << v->getName() << "\n");

    return true;
  }

  return false;
}
