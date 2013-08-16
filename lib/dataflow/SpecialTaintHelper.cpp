#include "SpecialTaintHelper.h"

SpecialTaintHelper::SpecialTaintValueMap SpecialTaintHelper::valueRegistry;
SpecialTaintHelper::FunctionToSpecialTaintMap SpecialTaintHelper::callRegistry;

const SpecialTaint& SpecialTaintHelper::getExternalTaints(const CallInst& call) {
  // Calling to a function pointer
  if (!call.getCalledFunction())
    return SpecialTaint::createNullTaint();

  SpecialTaintCache::iterator cacheEntry = cache.find(&call);
  if (cacheEntry != cache.end())
    return cacheEntry->second;

  const Function& func = *call.getCalledFunction();

  StringRef fName = func.getName();

  RegistryMap::iterator registryEntry = registry.find(fName);
  if (registryEntry != registry.end()) {
    SpecialTaintInstruction& inst = *registryEntry->second;
    SpecialTaint taint = inst.handleInstruction(call);

    valueRegistry.insert(std::make_pair(taint.specialTaintValue->getName(), taint.specialTaintValue));

    const Function* enclosingFunction = call.getParent()->getParent();
    std::set<SpecialTaint>& set = callRegistry[enclosingFunction];
    set.insert(taint);

    for (TaintSet::const_iterator a_i = taint.aliases.begin(), a_e = taint.aliases.end(); a_i != a_e; ++a_i) {
      const Value* v = *a_i;
      ValueSet& vs = aliases[v];
      vs.insert(taint.specialTaintValue);
      DEBUG(_logger.debug() << "alias list: " << *v << " <==> " << *taint.specialTaintValue << "\n");
    }

    return cache.insert(std::make_pair(&call, taint)).first->second;
  }

  return SpecialTaint::createNullTaint();
}
