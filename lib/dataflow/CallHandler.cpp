#include "CallHandler.h"
#include <cstring>
#include <sstream>
#include "TaintFile.h"
#include "IntrinsicHelper.h"
#include "FunctionProcessor.h"
#include "AliasHelper.h"


void CallHandler::handleInstructionInternal(const CallInst& callInst, TaintSet& taintSet) const {
  DEBUG(CTX.logger.debug() << " Handle CALL instruction:\n");
  const Function* callee = callInst.getCalledFunction();
  const Value* calleeValue = callInst.getCalledValue();
  DEBUG(CTX.logger.debug() << " Callee:" << *calleeValue << "\n");

  // Skip inline ASM for now
  if (callInst.isInlineAsm()) {
    DEBUG(CTX.logger.debug() << " Ignoring inline ASM. \n");
    return;
  }

  if (callee != NULL) {
    // We are calling to a 'normal' function
    handleFunctionCall(callInst, *callee, taintSet);
  } else {
    // We are calling to a function pointer.
    // Since we cannot determine all relisations of the pointer
    // (eg. if the function is a API function to be use from extern)
    // we need to use the same heuristic we use for external functions.
    ResultSet taintResults;
    buildMappingWithHeuristic(callInst, taintResults);
    processFunctionCallResultSet(callInst, *callInst.getCalledValue(), taintResults, taintSet);
    CTX.FI.setCallsFunctionPointer(true);

    // If the function to be called is tained
    // (eg. because the function is selected inside
    // of an if or switch, the return value is also
    // tainted.
    if (taintSet.contains(*calleeValue)) {
      taintSet.add(callInst);
      DEBUG(CTX.DOT.addRelation(*calleeValue, callInst));
    }
  }
}


void CallHandler::buildMappingForRecursiveCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) const {
  IF_PROFILING(long t = Helper::getTimestamp());

  writeMapForRecursive(callInst, func, CTX.setHelper.resultSet, taintResults);

  IF_PROFILING(CTX.logger.profile() << "buildMappingForRecursiveCall() took " << Helper::getTimestampDelta(t) << " µs\n");
}


void CallHandler::buildMappingForCircularReferenceCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) const {
  IF_PROFILING(long t = Helper::getTimestamp());

  // Start a second instance of the analysis to process 
  // the callee with the current partial map of the current
  // function. Tha partial map was already written to a file.
  FunctionProcessor& refFp = FunctionProcessor::from(CTX, func);
  refFp.suppressPrintTaints();
  refFp.setShouldWriteErrors(false);
  refFp.processFunction();

  ResultSet refResult = refFp.getResult();
  AnalysisState state = refFp.getAnalysisState();
  ProcessingState processingState = state.getProcessingState();

  DEBUG(CTX.logger.debug() << "build circular ref mapping for " << func.getName() << " -- funcProc result was: " << refFp.didFinish() << "\n");
  DEBUG(CTX.logger.debug() << "state was: " << processingState << "\n");

  if (!refFp.didFinish()) {
    CTX.analysisState.stopWithError("", processingState);

    if (processingState == ErrorMissingDefinition || processingState == Deferred) {
      DEBUG(CTX.logger.debug() << "Deferring because of missing definition in circular call.\n");
      CTX.analysisState.setProcessingState(Deferred);
      CTX.analysisState.setMissingDefinition(state.getMissingDefinition());
    }

    return;
  }

  writeMapForRecursive(callInst, func, refResult, taintResults);
  delete(&refFp);
  IF_PROFILING(CTX.logger.profile() << "buildMappingForCircularReferenceCall() took " << Helper::getTimestampDelta(t) << " µs\n");
}

void CallHandler::writeMapForRecursive(const CallInst& callInst, const Function& func, const ResultSet& results, ResultSet& taintResults) const {
  for (ResultSet::const_iterator i = results.begin(), e = results.end(); i != e; ++i) {
    DEBUG(CTX.logger.debug() << "found mapping: " << *i->first << " => " << *i->second << "\n");

    Value* inVal = NULL;
    Value* outVal = NULL;

    if (isa<GlobalVariable>(i->first)) {
      inVal = const_cast<Value*>(i->first);
    } else {
      int inPos = getArgumentPosition(func, *i->first);
      inVal = callInst.getArgOperand(inPos);
    }

    if (isa<GlobalVariable>(i->second)) {
      outVal = const_cast<Value*>(i->second);
    } else {
      int outPos = getArgumentPosition(func, *i->second);
      outVal = const_cast<Value*>(outPos >= 0 ? callInst.getArgOperand(outPos) : &callInst);
    }

    if (!inVal || !outVal) {
      CTX.analysisState.stopWithError("Could not resolve argument when processing function mapping: " 
          + i->first->getName().str() + " => " + i->second->getName().str() + " for " + func.getName().str(), Error);
    }

    taintResults.insert(make_pair(inVal, outVal));
  }

}


/**
 * For calls to function pointers or exterals we use conservative heuristic:
 * 1) Every parameter taints the return value
 * 2) Every parameter taints all out pointers
 */
void CallHandler::buildMappingWithHeuristic(const CallInst& callInst, ResultSet& taintResults) const {
  vector<const Value*> arguments;
  const size_t argCount = callInst.getNumArgOperands();

  arguments.reserve(argCount + 30);
  for (size_t j = 0; j < argCount; j++) {
    arguments.push_back(callInst.getArgOperand(j));
  }

  for (Module::const_global_iterator g_i = CTX.M.global_begin(), g_e = CTX.M.global_end(); g_i != g_e; ++g_i) {
    const GlobalVariable& g = *g_i;

    // Skip constants (eg. string literals)
    if (g.isConstant())
      continue;

    arguments.push_back(&g);
  }

  for (size_t i = 0; i < arguments.size(); i++) {
    const Value& source = *arguments.at(i);

    // Every argument taints the return value
    taintResults.insert(make_pair(&source, &callInst));
    DEBUG(CTX.logger.debug() << "Function pointers: inserting mapping " << i << " => -1\n");

    // Every argument taints other pointer arguments (out-arguments)
    for (size_t j = 0; j < argCount; j++) {
      const Value& sink = *arguments.at(j);

      if (&source != &sink && sink.getType()->isPointerTy()) {
        DEBUG(CTX.logger.debug() << "Function pointers: inserting mapping " << i << " => " << j << "\n");
        taintResults.insert(make_pair(&source, &sink));
      }
    }
  }

}


void CallHandler::handleFunctionCall(const CallInst& callInst, const Function& callee, TaintSet& taintSet) const {
  DEBUG(CTX.logger.debug() << " * Calling function `" << callee.getName() << "`\n");

  ResultSet taintResults;

  if (TaintFile::exists(callee) && !Helper::circleListContains(CTX.circularReferences[&CTX.F], callee)) {
    //
    // A 'normal' call where the mapping is read from file
    //
    IF_PROFILING(long t = Helper::getTimestamp());
    buildMappingFromTaintFile(callInst, callee, taintResults);
    IF_PROFILING(CTX.logger.profile() << " buildMappingFromTaintFile() took " << Helper::getTimestampDelta(t) << " µs\n");
  } else if (callee.size() == 0 || callInst.isInlineAsm()) {
    //
    // A call to EXTERNal function
    //
    DEBUG(CTX.logger.debug() << "calling to undefined external. using heuristic.\n");
    buildMappingWithHeuristic(callInst, taintResults);
    CTX.FI.setCallsExternal(true);
  } else if (&callee == &CTX.F) {
    //
    // A self-recursive call
    //
    IF_PROFILING(long t = Helper::getTimestamp());
    // Build intermediate results to be used in next iteration
    CTX.setHelper.buildResultSet();
    IF_PROFILING(CTX.logger.profile() << " buildResultSet() took " << Helper::getTimestampDelta(t) << "\n");

    buildMappingForRecursiveCall(callInst, callee, taintResults);
  } else if (callee.isIntrinsic()) {
    //
    // A call to an Intrinsic
    //
    DEBUG(CTX.logger.debug() << "handle intrinsic call: " << callee.getName() << "\n");

    FunctionTaintMap mapping;
    if (IntrinsicHelper::getMapping(callee, mapping)) {
      createResultSetFromFunctionMapping(callInst, callee, mapping, taintResults);
    } else {
      CTX.analysisState.stopWithError("No definition of intrinsic `" + callee.getName().str() + "`.\n", ErrorMissingIntrinsic);
    }
  } else if (Helper::circleListContains(CTX.circularReferences[&CTX.F], callee)) {
    //
    // A mutual-recursive call
    //
    DEBUG(CTX.logger.debug() << "calling with circular reference: " << CTX.F.getName() 
        << " (caller) <--> (callee) " << callee.getName() << "\n");

    if (TaintFile::exists(callee)) {
      buildMappingFromTaintFile(callInst, callee, taintResults);
    } else {
      // Write out currently available taint mapping of this function
      // because the dependent function needs this information.
      CTX.setHelper.buildResultSet();
      TaintFile::writeResult(CTX.F, CTX.setHelper.resultSet);

      buildMappingForCircularReferenceCall(callInst, callee, taintResults);

      // Make sure the intermediate results for this function
      // is removed or we carry along wrong taints!
      TaintFile::remove(CTX.F);
    }
  } else {
    DEBUG(CTX.logger.debug() << "Deferring `" << CTX.F.getName() << "` -- call to `" << callee << "` could not (yet) be evaulated.\n");
    CTX.analysisState.stopWithError("", Deferred);
    CTX.analysisState.setMissingDefinition(&callee);
  }

  processFunctionCallResultSet(callInst, callee, taintResults, taintSet);
}


/**
 * Transforms a raw mapping in the form of
 * 0 => 1, 1 => -1, ...
 * to a mapping where actual Values of the call are used
 *
 * -1 on the right hand side of => denotes the return value
 */
void CallHandler::createResultSetFromFunctionMapping(const CallInst& callInst, const Function& callee, const FunctionTaintMap& mapping, ResultSet& taintResults) const {
  IF_PROFILING(long t = Helper::getTimestamp());

  size_t calleeArgCount = callee.getArgumentList().size();
  size_t callArgCount = callInst.getNumArgOperands();

  DEBUG(CTX.logger.debug() << " Got " << mapping.size() << " taint mappings for " << callee.getName() << "\n");
  for (FunctionTaintMap::const_iterator i = mapping.begin(), e = mapping.end(); i != e; ++i) {
    int paramPos = i->sourcePosition;
    int retvalPos = i->sinkPosition;

    set<const Value*> sources;
    set<const Value*> sinks;

    DEBUG(CTX.logger.debug() << " Converting mapping: " << paramPos << " => " << retvalPos << "\n");

    // Build set for sources
    //

    if (paramPos == -2) {
      // VarArgs
      for (size_t i = calleeArgCount; i < callArgCount; i++) {
        sources.insert(callInst.getArgOperand(i));
      }
    } else if (paramPos == -3) {
      // Seems to be a global
      DEBUG(CTX.logger.debug() << " no position mapping. searching global: " << i->sourceName << "\n");
      const Value* glob = CTX.M.getNamedGlobal(i->sourceName);
      sources.insert(glob);
      DEBUG(CTX.logger.debug() << " using global: " << *glob << "\n");
    } else {
      // Normal arguments
      const Value* arg = callInst.getArgOperand(paramPos);
      sources.insert(arg);
    }

    DEBUG(CTX.logger.debug() << "processed source-mappings\n");

    // Build set for sinks
    // 
    
    if (retvalPos == -1) {
      // Return value
      sinks.insert(&callInst);
    } else if (retvalPos == -3) {
      // Seems to be a global
      const Value* glob = CTX.M.getNamedGlobal(i->sinkName);
      sinks.insert(glob);
      DEBUG(CTX.logger.debug() << " no position mapping. using global: " << *glob << "\n");
    } else if (retvalPos == -2) {
      // Varargs
      // These can also be taint sinks, namely if pointers
      // are put as varargs -- because we cannot detect this in
      // the callee, we have to handle it here.
      for (size_t i = calleeArgCount; i < callArgCount; i++) {
        if (callInst.getArgOperand(i)->getType()->isPointerTy())
          sinks.insert(callInst.getArgOperand(i));
      }
    } else {
      // Out pointer
      const Value* returnTarget = callInst.getArgOperand(retvalPos);
      sinks.insert(returnTarget);
    }

    DEBUG(CTX.logger.debug() << "processed sink-mappings\n");

    for (set<const Value*>::iterator so_i = sources.begin(), so_e = sources.end(); so_i != so_e; so_i++) {
      const Value* source = *so_i;
      for (set<const Value*>::iterator si_i = sinks.begin(), si_e = sinks.end(); si_i != si_e; si_i++) {
        const Value* sink = *si_i;

        taintResults.insert(make_pair(source, sink));
      }
    }
  }

  IF_PROFILING(CTX.logger.profile() << "createResultSetFromFunctionMapping() took " << Helper::getTimestampDelta(t) << " µs\n");
}


void CallHandler::processFunctionCallResultSet(const CallInst& callInst, const Value& callee, ResultSet& taintResults, TaintSet& taintSet) const {
  bool needToAddGraphNodeForFunction = false;
  DEBUG(CTX.logger.debug() << "Mapping to CallInst arguments. Got " << taintResults.size() << " mappings.\n");
  for (ResultSet::const_iterator i = taintResults.begin(), e = taintResults.end(); i != e; ++i) {
    const Value& in = *i->first;
    const Value& out = *i->second;

    DEBUG(CTX.logger.debug() << "Processing mapping: " << in.getName() << " => " << out.getName() << "\n");

    if (taintSet.contains(in)) {
      // Add graph arrows and function-node only if taints
      // were found. Otherwise the function-node would be
      // orphaned in the graph.
      needToAddGraphNodeForFunction = true;
      DEBUG(CTX.logger.debug() << "in is: " << in << "\n");
      stringstream reas("");

      if (isa<GlobalVariable>(in))
        DEBUG(reas << "in, via " << in.getName().str());
      else
        DEBUG(reas << "in, arg# " << getArgumentPosition(callInst, in));

      DEBUG(CTX.DOT.addRelation(in, callee, reas.str()));

      if (taintSet.contains(callee) && out.getType()->isPointerTy()) {
        DEBUG(CTX.DOT.addRelation(callee, out, "function-indirection"));
        taintSet.add(out);
      }

      taintSet.add(out);
      if (&out == &callInst) {
        DEBUG(CTX.DOT.addRelation(callee, callInst, "ret"));
      } else {
        stringstream outReas("");

        if (isa<GlobalVariable>(out))
          DEBUG(outReas << "out, via " << out.getName().str());
        else
          DEBUG(outReas << "out, arg# " << getArgumentPosition(callInst, out));

        DEBUG(CTX.DOT.addRelation(callee, out, outReas.str()));
      }

      AliasHelper::handleAliasing(CTX, out, taintSet);
    }
  }


  if (needToAddGraphNodeForFunction)
    DEBUG(CTX.DOT.addCallNode(callee));
}


/**
 * Search the argument position for the given Value in
 * the given CallInst.
 *
 * @return the position of the argument in this call, -3 if not found
 */
int CallHandler::getArgumentPosition(const CallInst& c, const Value& v) const {
  const size_t argCount = c.getNumArgOperands();

  for (size_t i = 0; i < argCount; ++i) {
    if (c.getArgOperand(i) == &v)
      return i;
  }

  return -3;
}


/**
 * Search the parameter position for the given Value in
 * the given Function.
 *
 * @return the position of the parameter in the corresponding Function, -3 if not found
 */
int CallHandler::getArgumentPosition(const Function& f, const Value& v) const {
  if (isa<ReturnInst>(v))
    return -1;

  if (isa<Argument>(v))
    return (cast<Argument>(v)).getArgNo();

  return -3;
}


void CallHandler::buildMappingFromTaintFile(const CallInst& callInst, const Function& callee, ResultSet& taintResults) const {
  // Caching call instruction arguments-to-value mappings
  // per-function. This is a benefit if blocks are inspected
  // several times. This is the case if we have more than
  // one function parameter or have loops in the CFG.
  // Caching must be done per call (not per function!) as
  // the arguments are different for each call.
  if (CTX.mappingCache.count(&callInst)) {
    taintResults = CTX.mappingCache[&callInst];
    DEBUG(CTX.logger.debug() << "Use mapping from cache\n");
    return;
  } 

  const FunctionTaintMap* mapping = TaintFile::getMapping(callee, CTX.logger);

  if (!mapping) {
    CTX.analysisState.stopWithError("Cannot find definition of `" + callee.getName().str() + "`.\n", ErrorMissingDefinition);
    CTX.analysisState.setMissingDefinition(&callee);
    DEBUG(CTX.logger.debug() << "Set missing: " << callee.getName() << "\n");
    return;
  }

  createResultSetFromFunctionMapping(callInst, callee, *mapping, taintResults);
  CTX.mappingCache.insert(make_pair(&callInst, taintResults));
}
