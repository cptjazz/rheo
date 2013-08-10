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
    map<const Value*, SpecialTaint> cache;

  public:
    SpecialTaintHelper(LLVMContext& context) : _llvmContext(context) { }

    const SpecialTaint getExternalTaints(CallInst& call, TaintSet& taints) {
      // Calling to a function pointer
      if (!call.getCalledFunction())
        return SpecialTaint::createNullTaint();

      if (cache.count(&call))
        return cache.find(&call)->second;
      
      const Function& func = *call.getCalledFunction();

      string fName = func.getName();
      if (registry.count(fName)) {
        SpecialTaintInstruction& inst = *registry[fName];
        SpecialTaint taint = inst.handleInstruction(call, taints);

        cache.insert(make_pair(&call, taint));
        return taint;
      }

      return SpecialTaint::createNullTaint();
    }

    void propagateTaintsFromCall(const CallInst& call, TaintSet& taintSet) {
      map<const Value*, SpecialTaint>::iterator result = cache.find(&call);

      WE SHOULD HAVE A LOOKUP TABLE FROM AFFECTED VALUES 
        TO SPECIAL TAINTS. WE COME HERE FOR EACH CALL AND CHECK
        IF ONE OF THE ARGS HAS A LINK TO A SPECIAL TAINT.

      if (result == cache.end())
        return;

      SpecialTaint st = result->second;

      const size_t callInstArgCount = call.getNumArgOperands();
      for (size_t i = 0; i < callInstArgCount; i++) {
        const Value& v = *call.getArgOperand(i);

        if (st.affectedValues.count(&v)) {
          taintSet.add(*st.value);
        }
      }
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
