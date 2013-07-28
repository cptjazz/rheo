#ifndef SPECIAL_TAINT_HELPER_H
#define SPECIAL_TAINT_HELPER_H

#include "Core.h"
#include "llvm/CodeGen/PseudoSourceValue.h"

class SpecialTaintHelper {
  public:
    static const Value* processExternalTaints(CallInst& call, TaintSet& taints) {
      // Calling to a function pointer
      if (!call.getCalledFunction())
        return NULL;
      
      const Function& func = *call.getCalledFunction();

      // TODO: better design :)
      //

      // new names must start with '+'
      if (func.getName().equals("fopen")) {
        // Newly created taint affects retval
        taints.add(call);

        //Value* newVal = UndefValue::get(Type::getLabelTy(call.getContext()));
        //newVal->setName("+fopen_FILE");
        Value* newVal = BasicBlock::Create(call.getContext(), "+fopen_FILE");
        //Value* newVal = new PseudoSourceValue(Value::UndefValueVal);
        //newVal->setName("+fopen_FILE");

        return newVal;
      }

      return NULL;
    }

    static bool hasSpecialTreatment(const Function& func) {
      return (func.getName().equals("fopen"));
    }

    static bool isSpecialTaintValue(const Value& v) {
      return v.getName().startswith("+");
    }
};

#endif // SPECIAL_TAINT_HELPER_H
