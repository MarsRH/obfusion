#include "OBFS/BogusControlFlow.h"

using namespace llvm;
using namespace OBFS;
using std::vector;

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

  errs() << "[BogusControlFlow] Collect BB " << blocks.size() << "\n";

  // 2. 为每个非入口基本块创建虚假分支
  for (BasicBlock *BB : blocks) {

    // 获取原基本块终止指令
    if (!BB->getTerminator()) continue;
    Instruction *origTerm = BB->getTerminator();

    if (BranchInst *br = dyn_cast<BranchInst>(origTerm)) {
      errs() << "[BogusControlFlow] BranchInst\n";
      // 仅处理无条件分支，使用不透明谓词
      if (!br->isConditional()) {
        errs() << "[BogusControlFlow] Not Conditional Br\n";
        // 创建不透明谓词
        IRBuilder<> builder(BB);
        if (Instruction *term = BB->getTerminator()) {
            builder.SetInsertPoint(term);
        }
        Value *bogusCmp = createBogusCmp(builder);
        errs() << "[BogusControlFlow] CreateBogusCMP ok\n";
        // 创建虚假分支 使用随机选择的基本块进行克隆
        BasicBlock *bogusBB = createCloneBasicBlock(blocks[rand() % blocks.size()]);
        errs() << "[BogusControlFlow] CopyBB ok\n";
        // 创建条件分支
        builder.CreateCondBr(bogusCmp, bogusBB, br->getSuccessor(0));
        origTerm->eraseFromParent();
      }
      // 条件分支不做处理
    } 
    else if (SwitchInst *sw = dyn_cast<SwitchInst>(origTerm)) {
      // 为Switch添加不可达块
      errs() << "[BogusControlFlow] SwitchInst\n";
      BasicBlock *bogusBB = createCloneBasicBlock(blocks[rand() % blocks.size()]);
      sw->addCase(ConstantInt::get(Type::getInt32Ty(f->getContext()), rand()), bogusBB);
    }
    else {
      // 其他情况使用默认处理 理论上不会到达该分支
      errs() << "[BogusControlFlow] err branch\n";
      IRBuilder<> builder(BB);
      if (Instruction *term = BB->getTerminator()) {
          builder.SetInsertPoint(term);
      }
      Value *bogusCmp = createBogusCmp(builder);
      BasicBlock *bogusBB = createCloneBasicBlock(blocks[rand() % blocks.size()]);
      builder.CreateCondBr(bogusCmp, bogusBB, BB->getNextNode());
    }
  }

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

Value* BogusControlFlow::createBogusCmp(IRBuilder<> &builder){
    // if((y < 10 || x * (x + 1) % 2 == 0))
    // 等价于 if(true)
    Module *M = builder.GetInsertBlock()->getModule();
    LLVMContext &Context = M->getContext();
    
    GlobalVariable *xptr = new GlobalVariable(*M, Type::getInt32Ty(Context), false, 
        GlobalValue::CommonLinkage, ConstantInt::get(Type::getInt32Ty(Context), 0), "x");
    GlobalVariable *yptr = new GlobalVariable(*M, Type::getInt32Ty(Context), false, 
        GlobalValue::CommonLinkage, ConstantInt::get(Type::getInt32Ty(Context), 0), "y");
    
    Value *x = builder.CreateLoad(Type::getInt32Ty(Context), xptr);
    Value *y = builder.CreateLoad(Type::getInt32Ty(Context), yptr);
    Value *cond1 = builder.CreateICmpSLT(y, ConstantInt::get(Type::getInt32Ty(Context), 10));
    Value *op1 = builder.CreateAdd(x, ConstantInt::get(Type::getInt32Ty(Context), 1));
    Value *op2 = builder.CreateMul(op1, x);
    Value *op3 = builder.CreateURem(op2, ConstantInt::get(Type::getInt32Ty(Context), 2));
    Value *cond2 = builder.CreateICmpEQ(op3, ConstantInt::get(Type::getInt32Ty(Context), 0));
    return builder.CreateOr(cond1, cond2);
}

PreservedAnalyses BogusControlFlow::run(Function &F, FunctionAnalysisManager &AM) {
  errs() << "[BogusControlFlow] Pass running on " << F.getName() << "\n";
  // DEBUG: 打印原始IR
  errs() << "[BogusControlFlow] Original IR:\n";
  F.print(errs());
  if (!F.isDeclaration() && BogusCF(&F)) {
    // DEBUG: 打印最终IR
    errs() << "[BogusControlFlow] IR:\n";
    F.print(errs());
    return PreservedAnalyses::none();
  }
  return PreservedAnalyses::all();
}
