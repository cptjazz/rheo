#ifndef CALL_HANDLER_H
#define CALL_HANDLER_H

#include "InstructionHandler.h"
#include "Helper.h"


class CallHandler : public InstructionHandlerTrait<CallInst> {

  public:
    CallHandler(InstructionHandlerContext& ctx)
      : InstructionHandlerTrait(Instruction::Call, ctx) { }

    void handleInstructionInternal(const CallInst& inst, TaintSet& taintSet) const;

private:
    void buildMappingFromTaintFile(const CallInst &callInst, const Function &callee, ResultSet &taintResults) const;
    void buildMappingForRecursiveCall(const CallInst &callInst, const Function &func, ResultSet &taintResults) const;
    void buildMappingForCircularReferenceCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) const;
    void buildMappingForUndefinedExternalCall(const CallInst& callInst, const Function& func, ResultSet& taintResults) const;
    void createResultSetFromFunctionMapping(const CallInst& callInst, const Function& callee, const FunctionTaintMap& mapping, ResultSet& taintResults) const;
    void processFunctionCallResultSet(const CallInst& callInst, const Value& callee, ResultSet& taintResults, TaintSet& taintSet) const;
    void handleFunctionPointerCallWithHeuristic(const CallInst& callInst, TaintSet& taintSet) const;
    void handleFunctionCall(const CallInst& callInst, const Function& callee, TaintSet& taintSet) const;
    void writeMapForRecursive(const CallInst& callInst, const Function& func, const ResultSet& results, ResultSet& taintResults) const;
    int getArgumentPosition(const CallInst& c, const Value& v) const;
    int getArgumentPosition(const Function& f, const Value& v) const;
};

#endif // CALL_HANDLER_H
