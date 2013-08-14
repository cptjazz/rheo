#include "SpecialTaintInstruction.h"

class FopenSpecialTaintInstruction : public SpecialTaintInstruction {
  public:
    FopenSpecialTaintInstruction(LLVMContext& context)
      : SpecialTaintInstruction(context, "fopen", Source | Sink) { }

    const SpecialTaint handleInstruction(const CallInst& call) {
      SpecialTaint& st = createSpecialTaint("fopen_FILE", call);

      addAffectedValue(st, call);

      return st;
    }
};
