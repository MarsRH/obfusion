#include "OBFS/Flattening.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;
using namespace OBFS;

PreservedAnalyses Flattening::run(Function &F, FunctionAnalysisManager &) {
  errs() << "[Flattening] Pass running on " << F.getName() << "\n";

  // 跳过声明和空函数
  if (F.isDeclaration() || F.empty()) {
    return PreservedAnalyses::all();
  }

  // 获取函数的所有基本块
  std::vector<BasicBlock*> origBBs;
  for (BasicBlock &BB : F) {
    origBBs.push_back(&BB);
  }

  // 跳过只有一个基本块的函数
  if (origBBs.size() <= 1) {
    return PreservedAnalyses::all();
  }

  // 移除第一个基本块(入口块)
  BasicBlock &entryBB = F.getEntryBlock();
  origBBs.erase(origBBs.begin());

  // 创建调度变量
  IRBuilder<> builder(&entryBB);
  AllocaInst *switchVar = builder.CreateAlloca(
    builder.getInt32Ty(), nullptr, "switchVar");

  // 设置初始值
  builder.SetInsertPoint(entryBB.getTerminator());
  builder.CreateStore(builder.getInt32(0), switchVar);

  // 创建调度块
  BasicBlock *dispatchBB = BasicBlock::Create(
    F.getContext(), "dispatchBB", &F);
  
  // 修改入口块的终止指令，跳转到调度块
  BranchInst *newBr = BranchInst::Create(dispatchBB);
  ReplaceInstWithInst(entryBB.getTerminator(), newBr);

  // 创建默认块(用于返回)
  BasicBlock *defaultBB = BasicBlock::Create(
    F.getContext(), "defaultBB", &F);
  builder.SetInsertPoint(defaultBB);
  builder.CreateRetVoid();

  // 创建switch指令
  builder.SetInsertPoint(dispatchBB);
  LoadInst *load = builder.CreateLoad(builder.getInt32Ty(), switchVar, "load");
  SwitchInst *switchInst = builder.CreateSwitch(load, defaultBB, origBBs.size());

  // 修改每个基本块的终止指令
  for (unsigned i = 0; i < origBBs.size(); ++i) {
    BasicBlock *bb = origBBs[i];
    
    // 设置当前块的case值
    switchInst->addCase(builder.getInt32(i), bb);

    // 修改终止指令
    if (BranchInst *br = dyn_cast<BranchInst>(bb->getTerminator())) {
      if (br->isUnconditional()) {
        // 无条件跳转改为设置switch变量并跳回dispatch
        builder.SetInsertPoint(br);
        builder.CreateStore(builder.getInt32(i + 1), switchVar);
        builder.CreateBr(dispatchBB);
        br->eraseFromParent();
      } else {
        // 条件跳转需要更复杂的处理(这里简化处理)
        builder.SetInsertPoint(br);
        builder.CreateStore(builder.getInt32(i + 1), switchVar);
        builder.CreateBr(dispatchBB);
        br->eraseFromParent();
      }
    } else if (isa<ReturnInst>(bb->getTerminator())) {
      // 返回指令保持不变
      continue;
    }
  }

  return PreservedAnalyses::all();
}
