#ifndef GETELEMENTPRT_HANDLER_H
#define GETELEMENTPRT_HANDLER_H

#include <sstream>
#include "InstructionHandler.h"

class GetElementPtrHandler : public InstructionHandlerTrait<GetElementPtrInst> {

  public:
    GetElementPtrHandler(GraphExporter& dot, const DominatorTree& dt, PostDominatorTree& pdt, raw_ostream& stream)
      : InstructionHandlerTrait(Instruction::GetElementPtr, dot, dt, pdt, stream) { }

    void handleInstructionInternal(const GetElementPtrInst& inst, TaintSet& taintSet) const {
      const Value& op = *inst.getPointerOperand();

      if (taintSet.contains(op)) {
        taintSet.add(inst);
        DOT.addRelation(op, inst, "indexer");
        DEBUG_LOG(" + Added GEP taint: `" << inst << "`\n");
      }

      for (size_t i = 0; i < inst.getNumIndices(); i++) {
        const Value& idx = *inst.getOperand(i + 1);

        if (taintSet.contains(idx)) {
          taintSet.add(inst);
          stringstream reason("");
          reason << "index #" << i;
          DOT.addRelation(idx, inst, reason.str());
          DEBUG_LOG(" ++ Added GEP INDEX: `" << idx << "`\n");
        }
      }
    }
};

#endif // GETELEMENTPRT_HANDLER_H
