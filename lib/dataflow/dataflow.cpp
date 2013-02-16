#include "llvm/Pass.h"
#include "llvm/Function.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/Casting.h"
#include "llvm/InstrTypes.h"
#include <map>
#include <set>
#include <algorithm>
#include <cstring>
#include <stdio.h>
#include "GraphExporter.h"


using namespace llvm;
using namespace std;

typedef set<Value*> TaintSet;
typedef map<Value*, TaintSet> RetMap;
typedef map<Argument*, TaintSet> ArgMap;
typedef pair<Argument*, Value*> TaintPair;
typedef set<TaintPair> ResultSet;

namespace {

  const unsigned int OUTPUT_RELEASE = 0;

  struct Dataflow : public FunctionPass {
    static char ID;

    Dataflow() : FunctionPass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<DominatorTree>();
      AU.addRequired<PostDominatorTree>();
    }

    virtual bool runOnFunction(Function &F) {
      DominatorTree& DT = getAnalysis<DominatorTree>();
      PostDominatorTree& PDT = getAnalysis<PostDominatorTree>();

      RetMap returnStatements;
      ArgMap arguments;
      ResultSet taints;

      arguments.clear();
      returnStatements.clear();

      printInstructions(F);

      GraphExporter* dot = new GraphExporter(F.getName());

      findReturnStatements(F, returnStatements, dot);
      findArguments(F, arguments, returnStatements, dot);

      ArgMap::iterator arg_i = arguments.begin();
      ArgMap::iterator arg_e = arguments.end();
      
      for(; arg_i != arg_e; ++arg_i) {
        Argument& arg = *arg_i->first;
        TaintSet l = arg_i->second;

        int iteration = 0;
        int newSetLength;
        int oldSetLength;

        do {
          oldSetLength = l.size();

          debug() << " ** Begin Iteration #" << iteration << "\n";
          buildTaintSetFor(F, arg, l, DT, PDT, dot);
          debug() << " ** End Iteration #" << iteration << "\n";

          newSetLength = l.size();
          debug() << " ** Tain set length:" << newSetLength << "\n";

          iteration++;
        } while (iteration < 10 && oldSetLength != newSetLength);

        intersectSets(arg, l, returnStatements, taints, dot);
      }

      delete(dot);

      writeTaints(F, taints);
      return false;
    }

    private:
    void intersectSets(Argument& arg, TaintSet argTaintSet, RetMap retStmts, ResultSet& taints, GraphExporter* dot) {
      RetMap::iterator ret_i = retStmts.begin();
      RetMap::iterator ret_e = retStmts.end();

      for (; ret_i != ret_e; ++ret_i) {
        Value& retval = *ret_i->first;
        TaintSet retTaintSet = ret_i->second;

        if (&retval == &arg) {
          debug() << "Skipping detected self-taint\n";
          continue;
        }

        debug() << "Ret-set for `" << retval << "`:\n",
        printSet(retTaintSet);
        debug() << "\n";

        TaintSet intersect;
        set_intersection(argTaintSet.begin(), argTaintSet.end(), retTaintSet.begin(), retTaintSet.end(), 
                         inserter(intersect, intersect.end()));

        if (intersect.begin() != intersect.end()) {
          addTaint(taints, arg, retval);
        }
      }
    }


    void buildTaintSetFor(Function& F, Value& arg, TaintSet& taintSet, DominatorTree& DT, PostDominatorTree& PDT, GraphExporter* dot) {
      debug() << " *** Creating taint set for argument `" << arg.getName() << "`\n";

      // Arg trivially taints itself.
      taintSet.insert(&arg);

      for (inst_iterator inst_i = inst_begin(F), inst_e = inst_end(F); inst_i != inst_e; ++inst_i) {
        Instruction& inst = cast<Instruction>(*inst_i);
        debug() << "Inspecting instruction: " << inst << "\n";

        if (isa<BranchInst>(inst))
          handleBranchInstruction(cast<BranchInst>(inst), taintSet, DT, PDT, dot);
        else if (isa<StoreInst>(inst))
          handleStoreInstruction(cast<StoreInst>(inst), taintSet, DT, dot);
        else
          handleInstruction(inst, taintSet, DT, dot);
      }

      debug() << "Taint set for arg `" << arg.getName() << "`:\n";
      printSet(taintSet);
      debug() << "\n";
    }

    void addTaint(ResultSet& taints, Argument& tainter, Value& taintee) {
      taints.insert(TaintPair(&tainter, &taintee));
    }

    bool setContains(TaintSet& taintSet, Value& val) {
      return taintSet.find(&val) != taintSet.end();
    }

    void writeTaints(Function& F, ResultSet& taints) {
      release() << "__taints:";
      release().write_escaped(F.getName()) << "(";
      bool isFirstTime = true;

      for (ResultSet::iterator i = taints.begin(), e = taints.end(); i != e; ++i) {
        Argument& arg = cast<Argument>(*i->first);
        Value& retval = cast<Value>(*i->second);

        release() << (isFirstTime ? "" : ", ") << arg.getName() << " => " << getValueNameOrDefault(retval);
        isFirstTime = false;
      }

      release() << ")\n";
    }

    void handleStoreInstruction(StoreInst& storeInst, TaintSet& taintSet, DominatorTree& DT, GraphExporter* dot) {
      Value& source = *storeInst.getOperand(0);
      Value& target = *storeInst.getOperand(1);

      debug() << " handle STORE instruction " << storeInst << "\n";
      if (setContains(taintSet, source) || handleBlockTainting(taintSet, storeInst, DT, dot)) {
        taintSet.insert(&target);
        dot->addRelation(source, target);
        debug() << "added STORE taint: " << source << " --> " << target << "\n";
      }

      /*if (!handleBlockTainting(taintSet, storeInst, DT, dot) && !setContains(taintSet, source)) {
        taintSet.erase(&target);
        debug() << "removed STORE taint due to non-taint overwrite: " << source << " --> " << target << "\n";
        dot->addRelation(source, target);
      }*/
      
      //handleBlockTainting(taintSet, storeInst, DT, dot));
    }

    void handleBranchInstruction(BranchInst& inst, TaintSet& taintSet, DominatorTree& DT, PostDominatorTree& PDT, GraphExporter* dot) {
      debug() << "  Inspecting branch instruction:\n";
      if (inst.isConditional()) {
        Value& cmp = *inst.getCondition();
        debug() << "    Cmp is: " << cmp << "\n";
   
        bool isConditionTainted = setContains(taintSet, cmp);
        if (isConditionTainted) {
          debug() << "    Condition seems tainted.\n";
          dot->addRelation(cmp, inst);
         
          BasicBlock* brTrue = inst.getSuccessor(0);
          // true branch is always tainted
          taintSet.insert(brTrue);
          dot->addRelation(inst, *brTrue);
          debug() << "added true branch to taint set:\n";
          debug() << *brTrue << "\n";

          if (inst.getNumSuccessors() == 2) {
            BasicBlock* brFalse = inst.getSuccessor(1);
            // false branch is only tainted if successor 
            // is not the same as jump target after true branch
            BasicBlock* target = PDT.findNearestCommonDominator(brFalse, brTrue);
            debug() << "Nearest Common Post-Dominator for tr/fa: " << *target << "\n";

            if (target != brFalse) {
              taintSet.insert(brFalse);
              dot->addRelation(inst, *brFalse);
              debug() << "added false branch to taint set:\n";
              debug() << *brFalse << "\n";
            }
          }
        }
      }

      handleBlockTainting(taintSet, inst, DT, dot);
    }
    
    void handleInstruction(Instruction& inst, TaintSet& taintSet, DominatorTree& DT, GraphExporter* dot) {
      for (size_t o_i = 0; o_i < inst.getNumOperands(); o_i++) {
         debug() << "  Inspecting operand #" << o_i << "\n";
         Value& operand = *inst.getOperand(o_i);
         if (setContains(taintSet, operand)) {
           taintSet.insert(&inst);
           dot->addRelation(operand, inst);

           debug() << "TAINTER: " << operand << " --> TAINTEE " << inst << "\n";
           debug() << "    Added " << inst << "\n";
        }
      }

      handleBlockTainting(taintSet, inst, DT, dot);
    }

    bool handleBlockTainting(TaintSet& taintSet, Instruction& inst, DominatorTree& DT, GraphExporter* dot) {
      debug() << "Handle BLOCK-tainting for " << inst << "\n";
      bool result = false;

      for (TaintSet::iterator s_i = taintSet.begin(), s_e = taintSet.end(); s_i != s_e; ++s_i) {
        if (! isa<BasicBlock>(*s_i))
          continue;

        BasicBlock& taintedBlock = cast<BasicBlock>(**s_i);
        BasicBlock& currentBlock = *inst.getParent();

        if (DT.dominates(&taintedBlock, &currentBlock)) {
          debug() << "dirty block `" << taintedBlock.getName() << "` dominates `" << currentBlock.getName() << "`\n";

          taintSet.insert(&currentBlock);
          dot->addRelation(taintedBlock, currentBlock);

          taintSet.insert(&inst);
          dot->addRelation(currentBlock, inst);

          debug() << "instruction tainted by dirty block: " << inst << "\n";
          result = true;
        }
      }

      return result;
    }

    raw_ostream& release() {
      return errs();
    }

    raw_ostream& debug() {
      if (OUTPUT_RELEASE)
        return nulls();

      return errs();
    }

    StringRef getValueNameOrDefault(Value& v) {
      if (isa<Argument>(v))
        return v.getName();
      else
        return "$_retval";
    }

    void findArguments(Function& F, ArgMap& args, RetMap& retStmts, GraphExporter* dot) {
      for (Function::arg_iterator i = F.arg_begin(), e = F.arg_end(); i != e; ++i) {
        Argument& arg = *i;
        bool isInOutNode = false;

        if (arg.getType()->isPointerTy()) {
          TaintSet retlist;
          findAllStoresAndLoadsForOutArgumentAndAddToSet(F, arg, retlist, dot);
          retStmts.insert(pair<Value*, TaintSet>(&arg, retlist));
          dot->addInOutNode(arg);
          isInOutNode = true;

          debug() << "added arg `" << arg.getName() << "` to out-list\n";
        }

        TaintSet l;
        l.insert(&arg);
        if (!isInOutNode)
          dot->addInNode(arg);

        args.insert(pair<Argument*, TaintSet>(&arg, l));
        debug() << "added arg `" << arg.getName() << "` to arg-list\n";
      }
    }

    void findAllStoresAndLoadsForOutArgumentAndAddToSet(Function& F, Value& arg, TaintSet& retlist, GraphExporter* dot) {
      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        Instruction& I = cast<Instruction>(*i);
        
        debug() << "inspecting: " << I << "\n";
        if (isa<StoreInst>(I) && I.getOperand(0) == &arg) {
          Value& op = cast<Value>(*I.getOperand(1));
          retlist.insert(&op);
          dot->addRelation(arg, op);
          debug() << "Found store for `" << arg.getName() << "` @ " << op << "\n";

          findAllStoresAndLoadsForOutArgumentAndAddToSet(F, op, retlist, dot);
        }
        
        if (isa<LoadInst>(I) && I.getOperand(0) == &arg) {
          Value& op = cast<Value>(I);
          retlist.insert(&op);
          dot->addRelation(op, arg);
          debug() << "Found load for `" << arg.getName() << "` @ " << op << "\n";
          
          findAllStoresAndLoadsForOutArgumentAndAddToSet(F, op, retlist, dot);
        }
      }
    }

    void printSet(set<Value*>& s) {
      for (TaintSet::iterator i = s.begin(), e = s.end(); i != e; ++i) {
        debug() << **i << " | ";
      }
    } 

    void findReturnStatements(Function& F, RetMap& retStmts, GraphExporter* dot) {
      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
          if (isa<ReturnInst>(*i)) {
            ReturnInst& r = cast<ReturnInst>(*i);
            TaintSet l;
            
            // skip 'return void'
            Value* retval = r.getReturnValue();
            if (retval) {
              l.insert(retval);
              retStmts.insert(pair<Value*, set<Value*> >(retval, l));
              dot->addOutNode(r);
              debug() << "Found ret-stmt: " << r << "\n";
            }
          }
      }
    }

    void printInstructions(Function &F) {
      debug() << "Instructions: \n";

      for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
        debug() << i->getParent() << " | " << &*i << " | " << (*i) << "\n";
      }
    }
  };
}

char Dataflow::ID = 0;
static RegisterPass<Dataflow> X("dataflow", "Data-flow analysis", true, true);
