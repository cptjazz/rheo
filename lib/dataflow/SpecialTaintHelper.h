#ifndef SPECIAL_TAINT_HELPER_H
#define SPECIAL_TAINT_HELPER_H

#include "Core.h"
#include "GraphExporter.h"
#include "Logger.h"
#include "SpecialTaintInstruction.h"
#include <map>

class SpecialTaintHelper {
  typedef std::map<const Value*, const SpecialTaint> SpecialTaintCache;
  typedef std::map<StringRef, SpecialTaintInstruction*> RegistryMap;
  typedef std::map<std::string, const Value*> SpecialTaintValueMap;
  typedef std::map<const Function*, std::set<SpecialTaint> > FunctionToSpecialTaintMap;
  typedef std::map<const Value*, ValueSet> AliasMap;

  private:
    RegistryMap registry;
    LLVMContext& _llvmContext;
    SpecialTaintCache cache;
    Logger& _logger;
    AliasMap aliases;

    static SpecialTaintValueMap valueRegistry;
    static FunctionToSpecialTaintMap callRegistry;

  public:
    SpecialTaintHelper(LLVMContext& context, Logger& logger)
      : _llvmContext(context), _logger(logger) { }

    const SpecialTaint& getExternalTaints(const CallInst& call);


    template<class T>
    void registerFunction() {
      SpecialTaintInstruction* handler = new T(_llvmContext);
      StringRef name = handler->getFunctionName();
      registry.insert(std::make_pair(name, handler));
    }

    inline bool hasSpecialTreatment(const Function& func) const {
      return registry.find(func.getName()) != registry.end();
    }

    inline bool isSpecialTaintValue(const Value& v) const {
      return v.getName().startswith("+");
    }

    inline const Value* getSpecialTaintValueByName(const Twine t) {
      std::string str = t.str();
      DEBUG(_logger.debug() << "searching for: " << str << "\n");

      for (SpecialTaintValueMap::iterator i = valueRegistry.begin(), e = valueRegistry.end(); i != e; ++i) {
        DEBUG(_logger.debug() << "elem: " << i->first << "\n");
      }

      SpecialTaintValueMap::iterator res = valueRegistry.find(str);
      return (res != valueRegistry.end())
        ? res->second
        : NULL;
    }

    inline std::set<SpecialTaint>& getCalledFunctionSpecialTaints(const Function* f) const {
      return callRegistry[f];
    }

    inline void reRegisterNestedFunctionSpecialTaints(const Function& f, std::set<SpecialTaint>& set) {
      callRegistry[&f].insert(set.begin(), set.end());
    }

    inline bool hasAlias(const Value* v) {
      DEBUG(_logger.debug() << "alias list size: " << aliases.size() << "\n");
      return (aliases.find(v) != aliases.end());
    }

    inline const ValueSet& getAliases(const Value* v) {
      return aliases[v];
    }
};

#endif // SPECIAL_TAINT_HELPER_H
