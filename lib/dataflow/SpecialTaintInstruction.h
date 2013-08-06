#ifndef SPECIALTAINTINSTRUCTION_H
#define SPECIALTAINTINSTRUCTION_H

#include "Core.h"
#include <string>

class SpecialTaintInstruction {
  private:
    LLVMContext& _llvmContext;
    const string _functionName;

  protected:
    const Value* createValueWithName(string name) {
        return BasicBlock::Create(_llvmContext, "+" + name);
    }

  public:
    SpecialTaintInstruction(LLVMContext& context, const string fname) 
      : _llvmContext(context), _functionName(fname) { }

    bool canHandle(const Function& func) const {
      return (func.getName().equals(_functionName));
    }

    string getFunctionName() const {
      return _functionName;
    }

    virtual const Value* handleInstruction(CallInst& call, TaintSet& taints) = 0;

};

#endif // SPECIALTAINTINSTRUCTION_H
