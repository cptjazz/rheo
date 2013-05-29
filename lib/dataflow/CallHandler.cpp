#include "CallHandler.h"
#include "TaintFile.h"
#include "IntrinsicHelper.h"
#include "FunctionProcessor.h"
#include <cstring>
#include <sstream>


void CallHandler::handleInstructionInternal(const CallInst& callInst, TaintSet& taintSet) const {
  CTX.logger.debug() << " Handle CALL instruction:\n";
  const Function* callee = callInst.getCalledFunction();
  const Value* calleeValue = callInst.getCalledValue();
  CTX.logger.debug() << " Callee:" << *calleeValue << "\n";

  // Skip inline ASM for now
  if (callInst.isInlineAsm()) {
    CTX.logger.debug() << " Ignoring inline ASM. \n";
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
    handleFunctionPointerCallWithHeuristic(callInst, taintSet);

    // If the function to be called is tained
    // (eg. because the function is selected inside
    // of an if or switch, the return value is also
    // tainted.
    if (taintSet.contains(*calleeValue)) {
      taintSet.add(callInst);
      CTX.DOT.addRelation(*calleeValue, callInst);
    }
  }
}


void CallHandler::buildMappingForRecursiveCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) const {
  IF_PROFILING(long t = Helper::getTimestamp());

  for (ResultSet::const_iterator i = CTX.setHelper.resultSet.begin(), e = CTX.setHelper.resultSet.end(); i != e; ++i) {
    int inPos = getArgumentPosition(func, *i->first);
    int outPos = getArgumentPosition(func, *i->second);

    Value* inVal = callInst.getArgOperand(inPos);
    const Value* outVal = outPos >= 0 ? callInst.getArgOperand(outPos) : &callInst;

    taintResults.insert(make_pair(inVal, outVal));
  }

  CTX.logger.profile() << "buildMappingForRecursiveCall() took " << Helper::getTimestampDelta(t) << " µs\n";
}


void CallHandler::buildMappingForCircularReferenceCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) const {
  IF_PROFILING(long t = Helper::getTimestamp());

  FunctionProcessor refFp(CTX.PASS, func, CTX.circularReferences, CTX.M, CTX.logger);
  refFp.suppressPrintTaints();
  refFp.setShouldWriteErrors(false);
  refFp.processFunction();

  ResultSet refResult = refFp.getResult();
  AnalysisState state = refFp.getAnalysisState();
  ProcessingState processingState = state.getProcessingState();

  CTX.logger.debug() << "build circular ref mapping for " << func.getName() << " -- funcProc result was: " << refFp.didFinish() << "\n";
  CTX.logger.debug() << "state was: " << processingState << "\n";

  if (!refFp.didFinish()) {
    CTX.analysisState.stopWithError("", processingState);

    if (processingState == ErrorMissingDefinition || processingState == Deferred) {
      CTX.logger.debug() << "Deferring because of missing definition in circular call.\n";
      CTX.analysisState.setProcessingState(Deferred);
      CTX.analysisState.setMissingDefinition(state.getMissingDefinition());
    }

    return;
  }

  for (ResultSet::const_iterator i = refResult.begin(), e = refResult.end(); i != e; ++i) {
    CTX.logger.debug() << "found mapping: " << *i->first << " => " << *i->second << "\n";
    int inPos = getArgumentPosition(func, *i->first);
    int outPos = getArgumentPosition(func, *i->second);

    Value* inVal = callInst.getArgOperand(inPos);
    const Value* outVal = outPos >= 0 ? callInst.getArgOperand(outPos) : &callInst;

    taintResults.insert(make_pair(inVal, outVal));
  }

  CTX.logger.profile() << "buildMappingForCircularReferenceCall() took " << Helper::getTimestampDelta(t) << " µs\n";
}


/**
 * For calls to Functions that are declared but not defined
 * in the given bc-assembly (= extern functions) we use a
 * conservative heuristic:
 * 1) Every parameter taints the return value
 * 2) Every parameter taints all out pointers
 */
void CallHandler::buildMappingForUndefinedExternalCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) const {
  const size_t argCount = callInst.getNumArgOperands();

  for (size_t i = 0; i < argCount; i++) {
    const Value* arg = callInst.getArgOperand(i);

    // Every argument taints the return value
    taintResults.insert(make_pair(arg, &callInst));
    CTX.logger.debug() << "undef-ext call: " << arg->getName() << " -> $_retval\n";

    size_t k = 0;
    for (Function::const_arg_iterator a_i = func.arg_begin(), a_e = func.arg_end(); a_i != a_e; a_i++) {
      const Argument& param = *a_i;
      Value* out = callInst.getArgOperand(k);
      k++;

      if (!out) {
        CTX.logger.error() << "arg #" << k << " was NULL\n";
        continue;
      }

      if (param.getType()->isPointerTy() && out != arg) {
        // Since it is a pointer it is a possible out-argument
        taintResults.insert(make_pair(arg, out));
        CTX.logger.debug() << "undef-ext call: " << arg->getName() << " -> " << out->getName() << "\n";
      }
    }
  }
}


/**
 * For calls to FunctionPointer we use conservative heuristic:
 * 1) Every parameter taints the return value
 * 2) Every parameter taints all out pointers
 */
void CallHandler::handleFunctionPointerCallWithHeuristic(const CallInst& callInst, TaintSet& taintSet) const {
  ResultSet taintResults;
  const size_t argCount = callInst.getNumArgOperands();

  for (size_t i = 0; i < argCount; i++) {
    const Value& source = *callInst.getArgOperand(i);

    // Every arguments taints the return value
    taintResults.insert(make_pair(&source, &callInst));
    CTX.logger.debug() << "Function pointers: inserting mapping " << i << " => -1\n";

    // Every argument taints other pointer arguments (out-arguments)
    for (size_t j = 0; j < argCount; j++) {
      const Value& sink = *callInst.getArgOperand(j);

      if (&source != &sink && sink.getType()->isPointerTy()) {
        CTX.logger.debug() << "Function pointers: inserting mapping " << i << " => " << j << "\n";
        taintResults.insert(make_pair(&source, &sink));
      }
    }
  }

  processFunctionCallResultSet(callInst, *callInst.getCalledValue(), taintResults, taintSet);
}


void CallHandler::handleFunctionCall(const CallInst& callInst, const Function& callee, TaintSet& taintSet) const {
  CTX.logger.debug() << " * Calling function `" << callee.getName() << "`\n";

  ResultSet taintResults;
  long t;

  if (TaintFile::exists(callee) && !Helper::circleListContains(CTX.circularReferences[&CTX.F], callee)) {
    t = Helper::getTimestamp();
    buildMappingFromTaintFile(callInst, callee, taintResults);
    CTX.logger.profile() << " buildMappingFromTaintFile() took " << Helper::getTimestampDelta(t) << " µs\n";
  } else if (callee.size() == 0 || callInst.isInlineAsm()) {
    // External functions
    CTX.logger.debug() << "calling to undefined external. using heuristic.\n";
    buildMappingForUndefinedExternalCall(callInst, callee, taintResults);
  } else if (&callee == &CTX.F) {
    // build intermediate taint sets
    t = Helper::getTimestamp();
    CTX.setHelper.buildResultSet();
    CTX.logger.profile() << " buildResultSet() took " << Helper::getTimestampDelta(t) << "\n";

    buildMappingForRecursiveCall(callInst, callee, taintResults);
  } else if (callee.isIntrinsic()) {
    CTX.logger.debug() << "handle intrinsic call: " << callee.getName() << "\n";

    FunctionTaintMap mapping;
    if (IntrinsicHelper::getMapping(callee, mapping)) {
      createResultSetFromFunctionMapping(callInst, callee, mapping, taintResults);
    } else {
      CTX.analysisState.stopWithError("No definition of intrinsic `" + callee.getName().str() + "`.\n", ErrorMissingIntrinsic);
    }
  } else if (Helper::circleListContains(CTX.circularReferences[&CTX.F], callee)) {
    CTX.logger.debug() << "calling with circular reference: " << CTX.F.getName() << " (caller) <--> (callee) " << callee.getName() << "\n";

    if (TaintFile::exists(callee)) {
      buildMappingFromTaintFile(callInst, callee, taintResults);
    } else {
      // Write out currently available taint mapping of this function
      // because the dependent function needs this information.
      CTX.setHelper.buildResultSet();
      TaintFile::writeResult(CTX.F, CTX.setHelper.resultSet);

      buildMappingForCircularReferenceCall(callInst, callee, taintResults);

      TaintFile::remove(CTX.F);
    }
  } else {
    CTX.logger.debug() << "Deferring `" << CTX.F.getName() << "` -- call to `" << callee << "` could not (yet) be evaulated.\n";
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

  CTX.logger.debug() << " Got " << mapping.size() << " taint mappings for " << callee.getName() << "\n";
  for (FunctionTaintMap::const_iterator i = mapping.begin(), e = mapping.end(); i != e; ++i) {
    int paramPos = i->first;
    int retvalPos = i->second;

    size_t calleeArgCount = callee.getArgumentList().size();
    size_t callArgCount = callInst.getNumArgOperands();

    set<const Value*> sources;
    set<const Value*> sinks;

    CTX.logger.debug() << " Converting mapping: " << paramPos << " => " << retvalPos << "\n";

    // Build set for sources
    if (paramPos == -2) {
      // VarArgs
      for (size_t i = calleeArgCount; i < callArgCount; i++) {
        sources.insert(callInst.getArgOperand(i));
      }
    } else {
      // Normal arguments
      const Value* arg = callInst.getArgOperand(paramPos);
      sources.insert(arg);
    }
    CTX.logger.debug() << "processed source-mappings\n";

    // Build set for sinks
    if (retvalPos == -1) {
      // Return value
      sinks.insert(&callInst);
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

    CTX.logger.debug() << "processed sink-mappings\n";

    for (set<const Value*>::iterator so_i = sources.begin(), so_e = sources.end(); so_i != so_e; so_i++) {
      const Value& source = **so_i;
      for (set<const Value*>::iterator si_i = sinks.begin(), si_e = sinks.end(); si_i != si_e; si_i++) {
        const Value& sink = **si_i;

        taintResults.insert(make_pair(&source, &sink));
      }
    }
  }

  CTX.logger.profile() << "createResultSetFromFunctionMapping() took " << Helper::getTimestampDelta(t) << " µs\n";
}


void CallHandler::processFunctionCallResultSet(const CallInst& callInst, const Value& callee, ResultSet& taintResults, TaintSet& taintSet) const {
  bool needToAddGraphNodeForFunction = false;
  for (ResultSet::const_iterator i = taintResults.begin(), e = taintResults.end(); i != e; ++i) {
    const Value& in = *i->first;
    const Value& out = *i->second;

    CTX.logger.debug() << "Processing mapping: " << in.getName() << " => " << out.getName() << "\n";

    if (taintSet.contains(in)) {
      // Add graph arrows and function-node only if taints
      // were found. Otherwise the function-node would be
      // orphaned in the graph.
      needToAddGraphNodeForFunction = true;
      CTX.logger.debug() << "in is: " << in << "\n";
      stringstream reas("");
      reas << "in, arg#" << getArgumentPosition(callInst, in);
      CTX.DOT.addRelation(in, callee, reas.str());

      if (taintSet.contains(callee) && out.getType()->isPointerTy()) {
        CTX.DOT.addRelation(callee, out, "function-indirection");
        taintSet.add(out);
      }

      taintSet.add(out);
      if (&out == &callInst) {
        CTX.DOT.addRelation(callee, callInst, "ret");
      } else {
        stringstream outReas("");
        outReas << "out, arg#" << getArgumentPosition(callInst, out);
        CTX.DOT.addRelation(callee, out, outReas.str());
      }

      // Value is a pointer, so the previous load is also tainted.
      if (isa<LoadInst>(out)) {
        Value* op = (cast<LoadInst>(out)).getOperand(0);
        taintSet.add(*op);
        CTX.logger.debug() << " ++ Added previous load: " << out << "\n";
        CTX.DOT.addRelation(*op, out, "load");
      }
    }
  }


  if (needToAddGraphNodeForFunction)
    CTX.DOT.addCallNode(callee);
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
  const FunctionTaintMap* mapping = TaintFile::getMapping(callee, CTX.logger);

  if (!mapping) {
    CTX.analysisState.stopWithError("Cannot find definition of `" + callee.getName().str() + "`.\n", ErrorMissingDefinition);
    CTX.analysisState.setMissingDefinition(&callee);
    CTX.logger.debug() << "Set missing: " << callee.getName() << "\n";
    return;
  }

  createResultSetFromFunctionMapping(callInst, callee, *mapping, taintResults);
}
