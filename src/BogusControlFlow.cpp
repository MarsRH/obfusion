#include "OBFS/BogusControlFlow.h"

using namespace llvm;
using namespace OBFS;
using std::vector;

bool BogusControlFlow::BogusCF(Function *f) {
  
  // 1. 收集所有基本块(跳过入口块、返回块和调用块)
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

  errs() << "[BogusControlFlow] Collect BB ok\n";

  // 2. 为每个非入口基本块创建虚假分支
  for (BasicBlock *BB : blocks) {

    // 创建不透明谓词 (这里使用简单实现)
    // Value *predicate = ConstantInt::get(Type::getInt1Ty(f->getContext()), 1);
    Value *bogusCmp = createBogusCmp(BB);
    
    errs() << "[BogusControlFlow] CreateBogusCMP ok\n";

    // 随机选择一个基本块进行复制(确保blocks不为空)
    if (blocks.empty()) continue;
    BasicBlock *bogusBB = createCloneBasicBlock(blocks[rand() % blocks.size()]);
    
    errs() << "[BogusControlFlow] CopyBB ok\n";

    // 清理复制块中的危险指令
    // for (Instruction &I : *bogusBB) {
    //   if (isa<CallInst>(&I) || isa<InvokeInst>(&I)) {
    //     I.eraseFromParent();
    //   }
    // }
    
    // 确保最后是分支指令
    // Instruction *term = bogusBB->getTerminator();
    // if (!term) {
    //   BranchInst::Create(BB, bogusBB);
    // } else if (!isa<BranchInst>(term)) {
    //   term->eraseFromParent();
    //   BranchInst::Create(BB, bogusBB);
    // }
    
    // 修改原基本块终止指令
    Instruction *origTerm = BB->getTerminator();
    IRBuilder<> builder(origTerm->getContext());
    builder.SetInsertPoint(origTerm);
    
    if (BranchInst *br = dyn_cast<BranchInst>(origTerm)) {
      if (br->isConditional()) {
        // 对于条件分支，组合原条件和新谓词
        Value *newCond = builder.CreateAnd(br->getCondition(), bogusCmp);
        builder.CreateCondBr(newCond, br->getSuccessor(0), bogusBB);
      } else {
        // 无条件分支直接使用新谓词
        builder.CreateCondBr(bogusCmp, bogusBB, br->getSuccessor(0));
      }
    } 
    else if (SwitchInst *sw = dyn_cast<SwitchInst>(origTerm)) {
      // 将Switch转换为条件分支
      Value *switchCond = sw->getCondition();
      BasicBlock *defaultBB = sw->getDefaultDest();
      builder.CreateCondBr(bogusCmp, bogusBB, defaultBB);
    }
    else {
      // 其他情况使用默认处理
      builder.CreateCondBr(bogusCmp, bogusBB, BB->getNextNode());
    }
    
    origTerm->eraseFromParent();
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

Value* BogusControlFlow::createBogusCmp(BasicBlock *insertAfter){
    // if((y < 10 || x * (x + 1) % 2 == 0))
    // 等价于 if(true)
    Module *M = insertAfter->getModule();
    InsertPosition *insertPos = new InsertPosition(insertAfter);
    LLVMContext &Context = M->getContext();
    GlobalVariable *xptr = new GlobalVariable(*M, Type::getInt32Ty(Context), false, GlobalValue::CommonLinkage, ConstantInt::get(Type::getInt32Ty(Context), 0), "x");
    GlobalVariable *yptr = new GlobalVariable(*M, Type::getInt32Ty(Context), false, GlobalValue::CommonLinkage, ConstantInt::get(Type::getInt32Ty(Context), 0), "y");
    LoadInst *x = new LoadInst(Type::getInt32Ty(Context), xptr, "", insertAfter);
    LoadInst *y = new LoadInst(Type::getInt32Ty(Context), yptr, "", insertAfter);
    ICmpInst *cond1 = new ICmpInst(*insertPos, CmpInst::ICMP_SLT, y, ConstantInt::get(Type::getInt32Ty(Context), 10));
    BinaryOperator *op1 = BinaryOperator::CreateAdd(x, ConstantInt::get(Type::getInt32Ty(Context), 1), "", insertAfter);
    BinaryOperator *op2 = BinaryOperator::CreateMul(op1, x, "", insertAfter);
    BinaryOperator *op3 = BinaryOperator::CreateURem(op2, ConstantInt::get(Type::getInt32Ty(Context), 2), "", insertAfter);
    ICmpInst *cond2 = new ICmpInst(*insertPos, CmpInst::ICMP_EQ, op3, ConstantInt::get(Type::getInt32Ty(Context), 0));
    return BinaryOperator::CreateOr(cond1, cond2, "", insertAfter);
}

PreservedAnalyses BogusControlFlow::run(Function &F, FunctionAnalysisManager &AM) {
  errs() << "[BogusControlFlow] Pass running on " << F.getName() << "\n";
  if (!F.isDeclaration() && BogusCF(&F)) {
    return PreservedAnalyses::none();
  }
  return PreservedAnalyses::all();
}
