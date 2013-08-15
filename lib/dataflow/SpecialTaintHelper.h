#ifndef SPECIAL_TAINT_HELPER_H
#define SPECIAL_TAINT_HELPER_H

#include "Core.h"
#include "SpecialTaintInstruction.h"
#include "GraphExporter.h"
#include <map>

class SpecialTaintHelper {
  typedef std::map<const Value*, const SpecialTaint> SpecialTaintMap;
  typedef std::map<StringRef, SpecialTaintInstruction*> RegistryMap;

  private:
    RegistryMap registry;
    LLVMContext& _llvmContext;
    SpecialTaintMap cache;
    Logger& _logger;

  public:
    SpecialTaintHelper(LLVMContext& context, Logger& logger)
      : _llvmContext(context), _logger(logger) { }

    const SpecialTaint& getExternalTaints(const CallInst& call) {
      // Calling to a function pointer
      if (!call.getCalledFunction())
        return SpecialTaint::createNullTaint();

      SpecialTaintMap::iterator cacheEntry = cache.find(&call);
      if (cacheEntry != cache.end())
        return cacheEntry->second;
      
      const Function& func = *call.getCalledFunction();

      StringRef fName = func.getName();

      RegistryMap::iterator registryEntry = registry.find(fName);
      if (registryEntry != registry.end()) {
        SpecialTaintInstruction& inst = *registryEntry->second;
        SpecialTaint taint = inst.handleInstruction(call);

        return cache.insert(std::make_pair(&call, taint)).first->second;
      }

      return SpecialTaint::createNullTaint();
    }

    void propagateTaintsFromCall(const Value& v, TaintSet& taintSet, GraphExporter& DOT) {
      DEBUG(_logger.debug() << "testing call-arg for special taint: " << v << "\n");
      for (SpecialTaintMap::iterator c_i = cache.begin(), c_e = cache.end(); c_i != c_e; ++c_i) {
        const SpecialTaint& st = c_i->second;

        DEBUG(_logger.debug() << "special taint affected values:\n");
        for (ValueSet::iterator v_i = st.affectedValues.begin(), v_e = st.affectedValues.end(); v_i != v_e; ++v_i)
          DEBUG(_logger.debug() << "aff val: " << *v_i << "\n");

        DEBUG(_logger.debug() << "testing special taint: " << *st.value << "\n");

        if (st.affectedValues.contains(v)) {
          DEBUG(_logger.debug() << "value `" << v << "` is affected!\n");
          taintSet.add(*st.value);
          IF_GRAPH(DOT.addRelation(v, *st.value));
          continue;
        }
      }
    }

    template<class T>
    void registerFunction() {
      SpecialTaintInstruction* handler = new T(_llvmContext);
      StringRef name = handler->getFunctionName();
      registry.insert(std::make_pair(name, handler));
    }

    bool hasSpecialTreatment(const Function& func) const {
      return registry.count(func.getName());
    }

    bool isSpecialTaintValue(const Value& v) const {
      return v.getName().startswith("+");
    }
};

#endif // SPECIAL_TAINT_HELPER_H
