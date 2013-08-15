#include "CallHandler.h"
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
    DEBUG(CTX.logger.debug() << " Handle normal function. \n");
    handleFunctionCall(callInst, *callee, taintSet);
  } else {
    // We are calling to a function pointer.
    // Since we cannot determine all relisations of the pointer
    // (eg. if the function is a API function to be use from extern)
    // we need to use the same heuristic we use for external functions.
    DEBUG(CTX.logger.debug() << " Handle function pointer. \n");

    ResultSet taintResults;
    buildMappingWithHeuristic(callInst, taintResults);
    processFunctionCallResultSet(callInst, *callInst.getCalledValue(), taintResults, taintSet);

    // If the function to be called is tained
    // (eg. because the function is selected inside
    // of an if or switch, the return value is also
    // tainted.
    if (taintSet.contains(*calleeValue)) {
      taintSet.add(callInst);
      IF_GRAPH(CTX.DOT.addRelation(*calleeValue, callInst));
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
  refFp.processFunction();

  ResultSet refResult = refFp.getResult();
  AnalysisState state = refFp.getAnalysisState();
  ProcessingState processingState = state.getProcessingState();
  bool didFinish = refFp.didFinish();
  delete (&refFp);

  DEBUG(CTX.logger.debug() << "build circular ref mapping for " << func.getName() << " -- funcProc result was: " << didFinish << "\n");
  DEBUG(CTX.logger.debug() << "state was: " << processingState << "\n");

  if (!didFinish) {
    CTX.analysisState.stopWithError("", processingState);

    if (processingState == ErrorMissingDefinition || processingState == Deferred) {
      DEBUG(CTX.logger.debug() << "Deferring because of missing definition in circular call.\n");
      CTX.analysisState.setProcessingState(Deferred);
      CTX.analysisState.setMissingDefinition(state.getMissingDefinition());
    }

    return;
  }

  writeMapForRecursive(callInst, func, refResult, taintResults);

  IF_PROFILING(CTX.logger.profile() << "buildMappingForCircularReferenceCall() took " << Helper::getTimestampDelta(t) << " µs\n");
  CTX.refreshDomTrees();
}

void CallHandler::writeMapForRecursive(const CallInst& callInst, const Function& func, const ResultSet& results, ResultSet& taintResults) const {
  const size_t callInstArgCount = callInst.getNumArgOperands();

  for (ResultSet::const_iterator i = results.begin(), e = results.end(); i != e; ++i) {
    DEBUG(CTX.logger.debug() << "found mapping: " << *i->first << " => " << *i->second << "\n");

    ValueSet sources;
    ValueSet sinks;

    if (isa<GlobalVariable>(i->first)) {
      sources.insert(i->first);
    } else {
      int inPos = getArgumentPosition(func, *i->first);

      if (inPos == -3) {
        // Not a global and not found in argument list,
        // must be a varargs array.
        for (size_t c = func.getArgumentList().size(); c < callInstArgCount; ++c) {
          sources.insert(callInst.getArgOperand(c));
        }
      } else {
        sources.insert(callInst.getArgOperand(inPos));
      }
    }

    if (isa<GlobalVariable>(i->second)) {
      sinks.insert(i->second);
    } else if (isa<ReturnInst>(i->second)) {
      sinks.insert(&callInst);
    } else {
      int outPos = getArgumentPosition(func, *i->second);

      if (outPos == -3) {
        // must be a varargs array.
        for (size_t c = func.getArgumentList().size(); c < callInstArgCount; ++c) {
          const Value* out = callInst.getArgOperand(c);
          if (out->getType()->isPointerTy())
            sinks.insert(out);
        }
      } else {
        sinks.insert(callInst.getArgOperand(outPos));
      }
    }


    for (ValueSet::const_iterator s_i = sources.begin(), s_e = sources.end(); s_i != s_e; ++s_i) {
      for (ValueSet::const_iterator d_i = sinks.begin(), d_e = sinks.end(); d_i != d_e; ++d_i) {
        const Value* inVal = *s_i;
        const Value* outVal = *d_i;

        DEBUG(CTX.logger.debug() << "added mapping: " << *inVal << " => " << *outVal << "\n");
        taintResults.insert(std::make_pair(inVal, outVal));
      }
    }
  }
}


/**
 * For calls to function pointers or exterals we use conservative heuristic:
 * 1) Every parameter taints the return value
 * 2) Every parameter taints all out pointers
 */
void CallHandler::buildMappingWithHeuristic(const CallInst& callInst, ResultSet& taintResults) const {
  std::vector<const Value*> arguments;
  const size_t argCount = callInst.getNumArgOperands();

  bool isExternal = callInst.getCalledFunction() != NULL;

  arguments.reserve(argCount + 30);
  for (size_t j = 0; j < argCount; j++) {
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
    int sourcePos = isa<GlobalVariable>(source) ? -3 : i;

    // Every argument taints the return value
    taintResults.insert(std::make_pair(&source, &callInst));
    DEBUG(CTX.logger.debug() << "Heuristic: inserting mapping " << sourcePos << " => -1\n");

    // Every argument taints other pointer arguments (out-arguments)
    for (size_t j = 0; j < argumentsCount; j++) {
      const Value& sink = *arguments.at(j);
      int sinkPos = isa<GlobalVariable>(sink) ? -3 : j;

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


void CallHandler::handleFunctionCall(const CallInst& callInst, const Function& callee, TaintSet& taintSet) const {
  DEBUG(CTX.logger.debug() << " * Calling function `" << callee.getName() << "`\n");

  ResultSet taintResults;

  // Caching call instruction arguments-to-value mappings
  // per-function. This is a benefit if blocks are inspected
  // several times. This is the case if we have more than
  // one function parameter or have loops in the CFG.
  // Caching must be done per call (not per function!) as
  // the arguments are different for each call.
  IF_PROFILING(long t = Helper::getTimestamp());
  if (CTX.mappingCache.count(&callInst)) {
    DEBUG(CTX.logger.debug() << "Use mapping from cache\n");

    ResultSet& res = CTX.mappingCache[&callInst];
    processFunctionCallResultSet(callInst, callee, res, taintSet);
    IF_PROFILING(CTX.logger.profile() << " processFCRS :: read 2 from cache took " << Helper::getTimestampDelta(t) << " µs\n");
    return;
  } 

  if (TaintFile::exists(callee) && !Helper::circleListContains(CTX.circularReferences[&CTX.F], callee)) {
    //
    // A 'normal' call where the mapping is read from file
    //
    IF_PROFILING(long t = Helper::getTimestamp());
    buildMappingFromTaintFile(callInst, callee, taintResults);
    IF_PROFILING(CTX.logger.profile() << " buildMappingFromTaintFile() took " << Helper::getTimestampDelta(t) << " µs\n");
  } else if (callee.isIntrinsic()) {
    //
    // A call to an Intrinsic
    //
    DEBUG(CTX.logger.debug() << "handle intrinsic call: " << callee.getName() << "\n");

    if (IntrinsicHelper::shouldIgnoreCall(callee)) {
      DEBUG(CTX.logger.debug() << "intrinsic call is marked to be ignored: " << callee.getName() << "\n");
      return;
    }

    FunctionTaintMap mapping;
    if (IntrinsicHelper::getMapping(callee, mapping)) {
      createResultSetFromFunctionMapping(callInst, callee, mapping, taintResults);
    } else {
      CTX.analysisState.stopWithError("No definition of intrinsic `" + callee.getName().str() + "`.\n", ErrorMissingIntrinsic);
    }
  } else if (callee.size() == 0 || callInst.isInlineAsm()) {
    //
    // A call to EXTERNal function
    //
    DEBUG(CTX.logger.debug() << "calling to undefined external. using heuristic.\n");
    buildMappingWithHeuristic(callInst, taintResults);
  } else if (&callee == &CTX.F) {
    //
    // A self-recursive call
    //
    IF_PROFILING(long t = Helper::getTimestamp());
    // Build intermediate results to be used in next iteration
    CTX.setHelper.buildResultSet();
    IF_PROFILING(CTX.logger.profile() << " buildResultSet() took " << Helper::getTimestampDelta(t) << "\n");

    buildMappingForRecursiveCall(callInst, callee, taintResults);
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
      TaintFile::writeTempResult(CTX.STH, CTX.F, CTX.setHelper.resultSet);

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

  const size_t calleeArgCount = callee.getArgumentList().size();
  const size_t callArgCount = callInst.getNumArgOperands();

  ValueSet sources;
  ValueSet sinks;

  DEBUG(CTX.logger.debug() << " Got " << mapping.size() << " taint mappings for " << callee.getName() << "\n");
  for (FunctionTaintMap::const_iterator i = mapping.begin(), e = mapping.end(); i != e; ++i) {
    const int paramPos = i->sourcePosition;
    const int retvalPos = i->sinkPosition;

    DEBUG(CTX.logger.debug() << " Converting mapping: " << paramPos << " => " << retvalPos << "\n");

    // Build set for sources
    //

    IF_PROFILING(long t2 = Helper::getTimestamp());
    sources.clear();
    if (paramPos == -2) {
      // VarArgs
      for (size_t i = calleeArgCount; i < callArgCount; ++i) {
        sources.insert(callInst.getArgOperand(i));
      }
    } else if (paramPos == -3) {
      // Seems to be a global
      DEBUG(CTX.logger.debug() << " no position mapping. searching global: " << i->sourceName << "\n");
      StringRef globName = i->sourceName.substr(1, i->sourceName.length() - 1);

      Value* glob = CTX.M.getNamedAlias(globName);

      if (glob == NULL)
        glob = CTX.M.getNamedValue(globName);

      if (glob == NULL)
        glob = CTX.M.getNamedGlobal(globName);

      assert("Global" && glob != NULL);

      sources.insert(glob);
      DEBUG(CTX.logger.debug() << " using global: " << *glob << "\n");
    } else {
      // Normal arguments
      const Value* arg = callInst.getArgOperand(paramPos);
      sources.insert(arg);
    }

    DEBUG(CTX.logger.debug() << "processed source-mappings\n");
    //IF_PROFILING(CTX.logger.profile() << "createResultSetFromFunctionMapping :: create sources took " << Helper::getTimestampDelta(t2) << " µs\n");

    // Build set for sinks
    // 
    
    IF_PROFILING(t2 = Helper::getTimestamp());
    sinks.clear();
    if (retvalPos == -1) {
      // Return value
      sinks.insert(&callInst);
    } else if (retvalPos == -3) {
      // Seems to be a global
      DEBUG(CTX.logger.debug() << " no position mapping. searching global: " << i->sinkName << "\n");
      StringRef globName = i->sinkName.substr(1, i->sinkName.length() - 1);
      
      const Value* glob = CTX.M.getNamedGlobal(globName);
      sinks.insert(glob);
      DEBUG(CTX.logger.debug() << " using global: " << *glob << "\n");
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
    //IF_PROFILING(CTX.logger.profile() << "createResultSetFromFunctionMapping :: create sinks took " << Helper::getTimestampDelta(t2) << " µs\n");

    for (ValueSet::iterator so_i = sources.begin(), so_e = sources.end(); so_i != so_e; ++so_i) {
      const Value* source = *so_i;
      for (ValueSet::iterator si_i = sinks.begin(), si_e = sinks.end(); si_i != si_e; ++si_i) {
        const Value* sink = *si_i;

        taintResults.insert(std::make_pair(source, sink));
      }
    }
  }

  IF_PROFILING(CTX.logger.profile() << "createResultSetFromFunctionMapping() took " << Helper::getTimestampDelta(t) << " µs\n");
}


void CallHandler::processFunctionCallResultSet(const CallInst& callInst, const Value& callee, ResultSet& taintResults, TaintSet& taintSet) const {
  DEBUG(CTX.logger.debug() << "Mapping to CallInst arguments. Got " << taintResults.size() << " mappings.\n");

  IF_PROFILING(long t1 = Helper::getTimestamp());
  // Store information if the in-value is
  // currently in taint-set. Do this for
  // all in-values and remove them from the taint-set.
  // We do this to simulate possible kills of the value
  // in the taint-mapping. If the value stays tainted,
  // it is re-added.
  TaintSet inTaintSet;
  for (ResultSet::const_iterator i = taintResults.begin(), e = taintResults.end(); i != e; ++i) {
    const Value& in = *i->first;
    assert("LHS must not be NULL" && i->first != NULL);

    if (!inTaintSet.contains(in) && taintSet.contains(in)) {
      inTaintSet.add(in);
      taintSet.remove(in);
    }
  }
  IF_PROFILING(CTX.logger.profile() << "PFCR 1 took " << Helper::getTimestampDelta(t1) << " µs\n");

  IF_PROFILING(t1 = Helper::getTimestamp());
  // Remove out-pointer and global-sinks.
  // Do this in separate loop to not disturb
  // the taintSet.contains used above.
  for (ResultSet::const_iterator i = taintResults.begin(), e = taintResults.end(); i != e; ++i) {
    const Value& out = *i->second;
    assert("RHS must not be NULL" && i->second != NULL);

    if (out.getType()->isPointerTy())
      taintSet.remove(out);
  }
  IF_PROFILING(CTX.logger.profile() << "PFCR 2 took " << Helper::getTimestampDelta(t1) << " µs\n");

  
  // Handle special taints for current call
  if (CTX.STH.isSpecialTaintValue(*CTX.currentArgument)) {
    const SpecialTaint& st = CTX.STH.getExternalTaints(callInst);
    taintSet.addAll(st.affectedValues);
  }

  IF_PROFILING(t1 = Helper::getTimestamp());
  for (ResultSet::const_iterator i = taintResults.begin(), e = taintResults.end(); i != e; ++i) {
    const Value& in = *i->first;
    const Value& out = *i->second;

    // Skip mapping if one of the parameters
    // is null. This happens when using 
    // taint-files with globals not available
    // in the currently inspected assembly.
    if (!i->first || !i->second)
      continue;

    DEBUG(CTX.logger.debug() << "Processing mapping: " << in.getName() << " => " << out.getName() << "\n");

    if (inTaintSet.contains(in)) {
      // Add graph arrows and function-node only if taints
      // were found. Otherwise the function-node would be
      // orphaned in the graph.
      IF_GRAPH(CTX.DOT.addCallNode(callee));
      DEBUG(CTX.logger.debug() << "in is: " << in << "\n");
      std::string s;
      raw_string_ostream reas(s);

      if (isa<GlobalVariable>(in))
        DEBUG(reas << "in, via " << Helper::getValueName(in));
      else
        DEBUG(reas << "in, arg# " << getArgumentPosition(callInst, in));

      IF_GRAPH(CTX.DOT.addRelation(in, callee, reas.str()));

      if (taintSet.contains(callee) && out.getType()->isPointerTy()) {
        IF_GRAPH(CTX.DOT.addRelation(callee, out, "function-indirection"));
        taintSet.add(out);
      }

      DEBUG(CTX.logger.debug() << " + Added " << out << "\n");
      taintSet.add(out);
      if (&out == &callInst) {
        IF_GRAPH(CTX.DOT.addRelation(callee, callInst, "ret"));
      } else {
        std::string s;
        raw_string_ostream outReas(s);

        if (isa<GlobalVariable>(out))
          DEBUG(outReas << "out, via " << Helper::getValueName(out));
        else
          DEBUG(outReas << "out, arg# " << getArgumentPosition(callInst, out));

        IF_GRAPH(CTX.DOT.addRelation(callee, out, outReas.str()));
      }

      AliasHelper::handleAliasing(CTX, &out, taintSet);
      CTX.STH.propagateTaintsFromCall(out, taintSet, CTX.DOT);
    }
  }

  IF_PROFILING(CTX.logger.profile() << "PFCR 3 took " << Helper::getTimestampDelta(t1) << " µs\n");
}


/**
 * Search the argument position for the given Value in
 * the given CallInst.
 *
 * @return the position of the argument in this call, -3 if not found
 */
inline int CallHandler::getArgumentPosition(const CallInst& c, const Value& v) const {
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
inline int CallHandler::getArgumentPosition(const Function& f, const Value& v) const {
  if (isa<ReturnInst>(v))
    return -1;

  if (const Argument* arg = dyn_cast<Argument>(&v))
    return arg->getArgNo();

  return -3;
}


void CallHandler::buildMappingFromTaintFile(const CallInst& callInst, const Function& callee, ResultSet& taintResults) const {

  const FunctionTaintMap* mapping = TaintFile::getMapping(callee, CTX.logger);

  if (!mapping) {
    CTX.analysisState.stopWithError("Cannot find definition of `" + callee.getName().str() + "`.\n", ErrorMissingDefinition);
    CTX.analysisState.setMissingDefinition(&callee);
    DEBUG(CTX.logger.debug() << "Set missing: " << callee.getName() << "\n");
    return;
  }

  createResultSetFromFunctionMapping(callInst, callee, *mapping, taintResults);
  CTX.mappingCache.insert(std::make_pair(&callInst, taintResults));
}
