#include "llvm/Analysis/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/Support/Casting.h"
#include "DTFail.h"


char DTFail::ID = 0;

static RegisterPass<DTFail> X("dtfail", "Failing DomTrees", false, false);

bool DTFail::runOnModule(Module &module) {

  for (int j = 0; j < 100000; j++) {
    for (Module::const_iterator i = module.begin(), e = module.end(); i != e; ++i) {
      const Function& f = *i;

      // Skip externals
      if (!f.size())
        continue;

      const BasicBlock* entryBlock = &f.getEntryBlock();
      const BasicBlock* lastBlock = &f.back();

      errs() << "Running on function: " << f.getName() << "\n";
      PostDominatorTree& pdt = getAnalysis<PostDominatorTree>(const_cast<Function&>(f));
      DominatorTree& dt = getAnalysis<DominatorTree>(const_cast<Function&>(f));

      for (Function::const_iterator f_i = f.begin(), f_e = f.end(); f_i != f_e; f_i++) {
        const BasicBlock* block = f_i;
        const DomTreeNode* dtNode = dt.getNode(const_cast<BasicBlock*>(block));
        const DomTreeNode* pdtNode = pdt.getNode(const_cast<BasicBlock*>(block));

        assert(NULL != dtNode && "DomTree Node must not be NULL");
        assert(NULL != pdtNode && "PostDomTree Node must not be NULL");

        assert(dt.findNearestCommonDominator(entryBlock, block) == entryBlock);
        assert(pdt.findNearestCommonDominator(const_cast<BasicBlock*>(lastBlock), const_cast<BasicBlock*>(block)) == lastBlock);
      }
    }
  }

  return false;
}
