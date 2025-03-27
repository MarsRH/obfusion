#include "OBFS/Flattening.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;
using namespace OBFS;

PreservedAnalyses Flattening::run(Function &F, FunctionAnalysisManager &) {
  errs() << "[Flattening] Pass running on " << F.getName() << "\n";
  return PreservedAnalyses::all();
}

bool Flattening::flatten(Function *f) {

  // 跳过声明和空函数
  if (f.isDeclaration() || f.empty()) {
    return false;
  }

  // 获取函数的所有基本块
  vector<BasicBlock *> origBB;
  for (BasicBlock &BB : f) {
    origBB.emplace_back(&BB);
    // 跳过包含调用指令的基本块
    if (isa<InvokeInst>(BB.getTerminator())) {
      return false;
    }
  }

  // 跳过只有一个基本块的函数
  if (origBB.size() <= 1) {
    return false;
  }

  // 去除第一个基本块
  origBB.erase(origBB.begin());

  // 如果函数的入口块是条件跳转, 则将其分割
  BasicBlock &entryBB = f->getEntryBlock();
  if (BranchInst *br = dyn_cast<BranchInst>(entryBB.getTerminator())) {
    if (br->isConditional() || entryBB.getTerminator()->getNumSuccessors() > 1) {
      BasicBlock::iterator i = entryBB.end();
      --i;

      if (entryBB.size() > 1) {
        --i;
      }

      BasicBlock *tmpBB = entryBB.splitBasicBlock(i, "first");
      origBB.insert(origBB.begin(), tmpBB);
    }
  }

  // 删除块末尾的跳转
  entryBB->getTerminator()->eraseFromParent();

  // 创建主循环
  BasicBlock *loopEntry = BasicBlock::Create(f->getContext(), "loopEntry", f);
  BasicBlock *loopEnd = BasicBlock::Create(f->getContext(), "loopEnd", f);
  BasicBlock *swDefault = BasicBlock::Create(f->getContext(), "switchDefault", f);

  // 创建调度变量
  IRBuilder<> entryBuilder(&entryBB);
  AllocaInst *switchVar = entryBuilder.CreateAlloca(
    entryBuilder.getInt32Ty(), nullptr, "switchVar");
  StoreInst *store = entryBuilder.CreateStore(
    entryBuilder.getInt32(0), switchVar);
  entryBuilder.CreateBr(loopEntry);

  // 创建调度块
  IRBuilder<> dispatchBuilder(loopEntry);
  LoadInst *load = dispatchBuilder.CreateLoad(switchVar, "switchVar");
  SwitchInst *switchI = dispatchBuilder.CreateSwitch(load, swDefault, origBB.size());
  BranchInst::Create(loopEnd, swDefault); // swDefault jump to loopEnd
  BranchInst::Create(loopEntry, loopEnd); // loopEnd jump to loopEntry

}