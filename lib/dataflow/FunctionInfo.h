#ifndef FUNCTION_INFO_H
#define FUNCTION_INFO_H

#include "Core.h"
#include "llvm/Function.h"


struct FunctionInfo {
  private:
    bool callsExternals;
    bool callsFunctionPointers;
    SmallPtrSet<const GlobalVariable*, 32> usedGlobals;

  public:
    FunctionInfo() {
      callsExternals = false;
      callsFunctionPointers = false;
    }

    bool getCallsExternal() const;
    void setCallsExternal(bool value);
    bool getCallsFunctionPointer() const;
    void setCallsFunctionPointer(bool value);
    void addGlobalUsage(const GlobalVariable& g);
    bool usesGlobal(const GlobalVariable& g);
};

typedef map<const Function*, FunctionInfo*> FunctionInfos;

#endif // FUNCTION_INFO_H
