#include "SpecialTaintInstruction.h"

// There are two kinds of special instructions
// 1) the instruction creates a handle that is used
//    by other functions. (eg. fopen)
// 2) the instruction does not use a handle (eg. printf)

class FopenSpecialTaintInstruction : public SpecialTaintInstruction {
  public:
    FopenSpecialTaintInstruction(LLVMContext& context)
      : SpecialTaintInstruction(context, "fopen", Source | Sink) { }

    const SpecialTaint handleInstruction(const CallInst& call) {
      SpecialTaint& st = createSpecialTaint("fopen_FILE", call);

      st.registerSelfAsSource();
      st.registerSelfAsSink();
      st.registerSink(call);
      st.registerSource(call);

      st.registerAlias(call);

      return st;
    }
};

class PrintfSpecialTaintInstruction : public SpecialTaintInstruction {
  public:
    PrintfSpecialTaintInstruction(LLVMContext& context)
      : SpecialTaintInstruction(context, "printf", Sink) { }

    const SpecialTaint handleInstruction(const CallInst& call) {
      SpecialTaint& st = createSpecialTaint("printf", call);

      st.registerSelfAsSink();

      const size_t callArgCount = call.getNumArgOperands();
      for (size_t i = 0; i < callArgCount; ++i)
        st.registerSource(*call.getArgOperand(i));

      return st;
    }
};

