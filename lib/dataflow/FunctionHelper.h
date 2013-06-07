#ifndef FUNCTION_HELPER_H
#define FUNCTION_HELPER_H

#include "Core.h"
#include "InstructionHandlerContext.h"


class FunctionHelper {
  private:
    void findGlobalsInFunction();
    bool usedByCallee(const GlobalVariable& g);
    SmallPtrSet<const GlobalVariable*, 32> _globals;
    InstructionHandlerContext& CTX;
    const Function& F;

  public:
    FunctionHelper(InstructionHandlerContext& ctx, const Function& f) : CTX(ctx), F(f) {
      findGlobalsInFunction();
    }

    bool usesGlobal(const GlobalVariable& g) {
      return _globals.count(&g) || usedByCallee(g); 
    }
};

#endif // FUNCTION_HELPER_H
