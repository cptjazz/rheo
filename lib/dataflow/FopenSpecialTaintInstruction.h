#include "SpecialTaintInstruction.h"

class FopenSpecialTaintInstruction : public SpecialTaintInstruction {
  public:
    FopenSpecialTaintInstruction(LLVMContext& context)
      : SpecialTaintInstruction(context, "fopen", Source | Sink) { }

    const SpecialTaint handleInstruction(const CallInst& call, TaintSet& taints) {
      SpecialTaint& st = createSpecialTaint("fopen_FILE", call);

      addAffectedValue(taints, st, call);

      return st;
    }
};
