#include "SpecialTaintInstruction.h"

class FopenSpecialTaintInstruction : public SpecialTaintInstruction {
  public:
    FopenSpecialTaintInstruction(LLVMContext& context)
      : SpecialTaintInstruction(context, "fopen") { }

    virtual const Value* handleInstruction(CallInst& call, TaintSet& taints) {
      taints.add(call);
      return createValueWithName("fopen_FILE");
    }
};
