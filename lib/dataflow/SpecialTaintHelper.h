#ifndef SPECIAL_TAINT_HELPER_H
#define SPECIAL_TAINT_HELPER_H

#include "Core.h"
#include "SpecialTaintInstruction.h"
#include "llvm/CodeGen/PseudoSourceValue.h"
#include <map>

class SpecialTaintHelper {
  private:
    map<string, SpecialTaintInstruction*> registry;
    LLVMContext& _llvmContext;

  public:
    SpecialTaintHelper(LLVMContext& context) : _llvmContext(context) { }

    const Value* processExternalTaints(CallInst& call, TaintSet& taints) {
      // Calling to a function pointer
      if (!call.getCalledFunction())
        return NULL;
      
      const Function& func = *call.getCalledFunction();

      string fName = func.getName();
      if (registry.count(fName)) {
        SpecialTaintInstruction& inst = *registry[fName];
        return inst.handleInstruction(call, taints);
      }

      return NULL;
    }

    template<class T>
    void registerFunction() {
      SpecialTaintInstruction* handler = new T(_llvmContext);
      string name = handler->getFunctionName();
      registry.insert(make_pair(name, handler));
    }

    bool hasSpecialTreatment(const Function& func) const {
      return registry.count(func.getName());
    }

    bool isSpecialTaintValue(const Value& v) const {
      return v.getName().startswith("+");
    }
};

#endif // SPECIAL_TAINT_HELPER_H
