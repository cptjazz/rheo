#include "FunctionInfo.h"

bool FunctionInfo::getCallsFunctionPointer() const
{
    return callsFunctionPointers;
}

void FunctionInfo::setCallsFunctionPointer(bool value)
{
    callsFunctionPointers = value;
}

bool FunctionInfo::getCallsExternal() const
{
    return callsExternals;
}

void FunctionInfo::setCallsExternal(bool value)
{
    callsExternals = value;
}

void FunctionInfo::addGlobalUsage(const GlobalVariable& g) {
  usedGlobals.insert(&g);
}

bool FunctionInfo::usesGlobal(const GlobalVariable& g) {
  return usedGlobals.count(&g);
}
