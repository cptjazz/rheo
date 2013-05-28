#ifndef BRANCH_HANDLER_H
#define BRANCH_HANDLER_H

#include <sstream>
#include "InstructionHandler.h"

class BranchHandler : public InstructionHandlerTrait<BranchInst> {

  public:
    BranchHandler(GraphExporter& dot, const DominatorTree& dt, PostDominatorTree& pdt, raw_ostream& stream)
      : InstructionHandlerTrait(Instruction::Br, dot, dt, pdt, stream) { }

    void handleInstructionInternal(const PHINode& inst, TaintSet& taintSet) const {
      DEBUG_LOG(" Handle BRANCH instruction: " << inst << "\n");

      if (inst.isConditional()) {
        const Value& cmp = *inst.getCondition();
        DEBUG_LOG(" = Compare instruction is: " << cmp << "\n");

        bool isConditionTainted = taintSet.contains(cmp);
        if (isConditionTainted) {
          DOT.addRelation(cmp, inst, "condition");

          const BasicBlock& brTrue = *inst.getSuccessor(0);
          const BasicBlock& brFalse = *inst.getSuccessor(1);

          const BasicBlock& join = *const_cast<const BasicBlock*>(PDT.findNearestCommonDominator(const_cast<BasicBlock*>(&brFalse), const_cast<BasicBlock*>(&brTrue)));

          DEBUG_LOG("   Nearest Common Post-Dominator for tr/fa: " << join << "\n");

          // true branch is always tainted
          taintSet.add(brTrue);
          DOT.addBlockNode(brTrue);
          DOT.addRelation(inst, brTrue, "br-true");
          DEBUG_LOG(" + Added TRUE branch to taint set:\n");
          DEBUG_LOG(brTrue.getName() << "\n");

          followTransientBranchPaths(brTrue, join, taintSet);

          // false branch is only tainted if successor 
          // is not the same as jump target after true branch
          if (&join != &brFalse) {
            taintSet.add(brFalse);
            DOT.addBlockNode(brFalse);
            DOT.addRelation(inst, brFalse, "br-false");
            DEBUG_LOG(" + Added FALSE branch to taint set:\n");
            DEBUG_LOG(brFalse.getName() << "\n");

            followTransientBranchPaths(brFalse, join, taintSet);
          }
        }
      } else {
        if (taintSet.hasChanged()) {
          const BasicBlock* target = inst.getSuccessor(0);
          enqueueBlockToWorklist(target);
          DEBUG_LOG("Added block " << target->getName() << " for reinspection due to UNCONDITIONAL JUMP\n");
        }
      }
    }
};

#endif // BRANCH_HANDLER_H
