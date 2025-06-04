#include "OBFS/BogusControlFlow.h"
#include "OBFS/OpaquePredicates.h"

using namespace llvm;
using namespace OBFS;
using std::vector;

#define DEBUG_TYPE "BogusControlFlow"

PreservedAnalyses BogusControlFlow::run(Function &F, FunctionAnalysisManager &AM) {
  outs()<< "\n" << "[BogusControlFlow] Pass running on " << F.getName() << "\n";
  // DEBUG: 打印原始IR
  DEBUG_WITH_TYPE(DEBUG_TYPE, 
    dbgs() << "[BogusControlFlow] Original IR:\n"; 
    F.print(dbgs()););

  if (!F.isDeclaration() && BogusCF(&F)) {
    // DEBUG: 打印最终IR
    DEBUG_WITH_TYPE(DEBUG_TYPE, 
      dbgs() << "[BogusControlFlow] Final IR After Bogus Control Flow:\n"; 
      F.print(dbgs()););
  
    return PreservedAnalyses::none();
  }
  return PreservedAnalyses::all();
}

bool BogusControlFlow::BogusCF(Function *f) {
  
  // 收集所有基本块(跳过入口块、返回块和调用块)
  vector<BasicBlock*> blocks;
  BasicBlock &entryBB = f->getEntryBlock();
  for (BasicBlock &BB : *f) {
    if (&BB == &entryBB) {
      continue;
    }
    
    // 跳过返回块
    if (isa<ReturnInst>(BB.getTerminator())) {
      continue;
    }
    
    // 跳过调用块(包含CallInst或InvokeInst)
    bool hasCall = false;
    for (const Instruction &I : BB) {
      if (isa<CallInst>(&I) || isa<InvokeInst>(&I)) {
        hasCall = true;
        break;
      }
    }
    if (hasCall) {
      continue;
    }
    
    blocks.push_back(&BB);
  }

  outs() << "[BogusControlFlow] Collected " << blocks.size() << " basic blocks" << "\n";

  // 为每个非入口基本块创建虚假分支
  for (BasicBlock *BB : blocks) {

    // 获取原基本块终止指令
    if (!BB->getTerminator()) continue;
    Instruction *origTerm = BB->getTerminator();

    if (BranchInst *br = dyn_cast<BranchInst>(origTerm)) {
      // dbgs() << "[BogusControlFlow] BranchInst\n";
      // 仅处理无条件分支，使用不透明谓词
      if (!br->isConditional()) {
        // dbgs() << "[BogusControlFlow] Not Conditional Br\n";
        // 创建不透明谓词
        IRBuilder<> builder(BB);
        if (Instruction *term = BB->getTerminator()) {
            builder.SetInsertPoint(term);
        }
        Value *bogusCmp = OBFS::opaquepredicate->generate(builder);
        // Value *bogusCmp = createBogusCmp(builder);
        // dbgs() << "[BogusControlFlow] CreateBogusCMP ok\n";
        // 创建虚假分支 使用随机选择的基本块进行克隆
        BasicBlock *bogusBB = createCloneBasicBlock(blocks[rand() % blocks.size()]);
        // dbgs() << "[BogusControlFlow] CopyBB ok\n";
        // 创建条件分支
        builder.CreateCondBr(bogusCmp, br->getSuccessor(0), bogusBB);
        origTerm->eraseFromParent();
      }
      // 条件分支不做处理
    } 
    else if (SwitchInst *sw = dyn_cast<SwitchInst>(origTerm)) {
      // 为Switch添加不可达块
      // dbgs() << "[BogusControlFlow] SwitchInst\n";
      BasicBlock *bogusBB = createCloneBasicBlock(blocks[rand() % blocks.size()]);
      sw->addCase(ConstantInt::get(Type::getInt32Ty(f->getContext()), rand()), bogusBB);
    }
    else {
      // 其他情况使用默认处理 理论上不会到达该分支
      // errs() << "[BogusControlFlow] err branch\n";
      IRBuilder<> builder(BB);
      if (Instruction *term = BB->getTerminator()) {
          builder.SetInsertPoint(term);
      }
      Value *bogusCmp = OBFS::opaquepredicate->generate(builder);
      // Value *bogusCmp = createBogusCmp(builder);
      BasicBlock *bogusBB = createCloneBasicBlock(blocks[rand() % blocks.size()]);
      builder.CreateCondBr(bogusCmp, bogusBB, BB->getNextNode());
    }
  }

  outs() << "[BogusControlFlow] Bogus control flow added successfully\n";
  return true;
}

BasicBlock* BogusControlFlow::createCloneBasicBlock(BasicBlock *BB) {
    // 克隆之前先修复所有逃逸变量
    vector<Instruction*> origReg;
    BasicBlock &entryBB = BB->getParent()->getEntryBlock();
    for(Instruction &I : *BB){
        if(!(isa<AllocaInst>(&I) && I.getParent() == &entryBB) 
            && I.isUsedOutsideOfBlock(BB)){
            origReg.push_back(&I);
        }
    }
    for(Instruction *I : origReg){
        DemoteRegToStack(*I, entryBB.getTerminator());
    }
    
    ValueToValueMapTy VMap;
    BasicBlock *cloneBB = CloneBasicBlock(BB, VMap, "cloneBB", BB->getParent());
    // 对克隆基本块的引用进行修复
    for(Instruction &I : *cloneBB){
        for(unsigned i = 0;i < I.getNumOperands();i ++){
            Value *V = MapValue(I.getOperand(i), VMap);
            if(V){
                I.setOperand(i, V);
            }
        }
    }
    return cloneBB;
}