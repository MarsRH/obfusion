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

  // 设置初始值 - 确保entryBB有终止指令
  if (Instruction *term = entryBB.getTerminator()) {
    builder.SetInsertPoint(term);
  } else {
    builder.SetInsertPoint(&entryBB);
  }
  builder.CreateStore(builder.getInt32(0), switchVar);

  // 创建调度块
  BasicBlock *dispatchBB = BasicBlock::Create(
    F.getContext(), "dispatchBB", &F);
  
  // 修改入口块的终止指令，跳转到调度块
  if (Instruction *term = entryBB.getTerminator()) {
    BranchInst *newBr = BranchInst::Create(dispatchBB);
    ReplaceInstWithInst(term, newBr);
  } else {
    builder.SetInsertPoint(&entryBB);
    builder.CreateBr(dispatchBB);
  }

  // 创建默认块(用于返回)
  BasicBlock *defaultBB = BasicBlock::Create(
    F.getContext(), "defaultBB", &F);
  builder.SetInsertPoint(defaultBB);
  
  // 保留原始返回指令的类型
  if (F.getReturnType()->isVoidTy()) {
    builder.CreateRetVoid();
  } else {
    // 对于非void返回类型，需要创建适当的默认返回值
    builder.CreateRet(UndefValue::get(F.getReturnType()));
  }

  // 创建switch指令
  builder.SetInsertPoint(dispatchBB);
  LoadInst *load = builder.CreateLoad(builder.getInt32Ty(), switchVar, "load");
  SwitchInst *switchInst = builder.CreateSwitch(load, defaultBB, origBBs.size());

  // 修改每个基本块的终止指令
  for (unsigned i = 0; i < origBBs.size(); ++i) {
    BasicBlock *bb = origBBs[i];
    
    // 设置当前块的case值
    switchInst->addCase(builder.getInt32(i), bb);

    // 跳过没有终止指令的基本块
    if (!bb->getTerminator()) continue;

    // 修改终止指令
    if (BranchInst *br = dyn_cast<BranchInst>(bb->getTerminator())) {
      builder.SetInsertPoint(br);
      if (br->isUnconditional()) {
        // 无条件跳转改为设置switch变量并跳回dispatch
        builder.CreateStore(builder.getInt32(i + 1), switchVar);
        builder.CreateBr(dispatchBB);
        br->eraseFromParent();
      } else {
        // 条件跳转需要更复杂的处理(这里简化处理)
        // 可能需要创建PHI节点来正确处理条件分支
        builder.CreateStore(builder.getInt32(i + 1), switchVar);
        builder.CreateBr(dispatchBB);
        br->eraseFromParent();
      }
    } else if (isa<ReturnInst>(bb->getTerminator())) {
      // 返回指令保持不变
      continue;
    } else if (isa<UnreachableInst>(bb->getTerminator())) {
      // 不可达指令保持不变
      continue;
    }
  }

  return PreservedAnalyses::all();
}