#include "FunctionHelper.h"
#include "llvm/Support/InstIterator.h"

void FunctionHelper::findGlobalsInFunction()
{
  // Search all operands of all instructions for usages of the global.
  // g.isUsedInBasicBlock(..) does not work, as it only counts the 
  // 'uses' (as in use-def) and no definitions (eg. right hands of STOREs)
  for (const_inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    const size_t opCount = i->getNumOperands();
    
    for (size_t j = 0; j < opCount; j++) {
      const Value* op = i->getOperand(j);
      if (isa<GlobalVariable>(op)) {
        _globals.insert(cast<GlobalVariable>(op));
      }
    }
  }

  DEBUG(CTX.logger.debug() << "Found " << _globals.size() << " globals in " << F.getName() << "\n");
}

bool FunctionHelper::usedByCallee(const GlobalVariable& g) {
  
  for (const_inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
    if (!isa<CallInst>(*i))
      continue;

    const CallInst& call = cast<CallInst>(*i);

    const Function* callee = call.getCalledFunction();

    // Function Pointer
    if (!callee)
      return true;

    // External
    if (!callee->size())
      return true;

    FunctionInfo& info = *CTX.functionInfos[callee];

    if (info.getCallsExternal() || info.getCallsFunctionPointer())
      return true;

    if (info.usesGlobal(g))
      return true;
  }

  return false;
}
