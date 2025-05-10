#include "OBFS/BogusCFG.h"

using namespace llvm;
using namespace OBFS;

bool BogusCFG::bogusCFG(Function *f) {
  // 1. 收集所有基本块(跳过入口块)
  std::vector<BasicBlock*> blocks;
  BasicBlock &entryBB = f->getEntryBlock();
  for (BasicBlock &BB : *f) {
    if (&BB != &entryBB) {
      blocks.push_back(&BB);
    }
  }

  // 2. 为每个非入口基本块创建虚假分支
  for (BasicBlock *BB : blocks) {
    // 创建不透明谓词 (这里使用简单实现)
    Value *predicate = ConstantInt::get(Type::getInt1Ty(f->getContext()), 1);
    
    // 创建虚假基本块
    BasicBlock *bogusBB = BasicBlock::Create(
        f->getContext(), "bogus", f);
    BranchInst::Create(BB, bogusBB);
    
    // 修改原基本块终止指令
    Instruction *term = BB->getTerminator();
    IRBuilder<> builder(term);
    builder.CreateCondBr(predicate, BB->getNextNode(), bogusBB);
    term->eraseFromParent();
  }

  return true;
}

PreservedAnalyses BogusCFG::run(Function &F, FunctionAnalysisManager &AM) {
  errs() << "[BogusCFG] Pass running on " << F.getName() << "\n";
  if (!F.isDeclaration()) {
    bogusCFG(&F);
  }
  return PreservedAnalyses::all();
}
