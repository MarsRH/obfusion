#include "OBFS/Flattening.h"

using namespace llvm;
using namespace OBFS;
using std::vector;

PreservedAnalyses Flattening::run(Function &F, FunctionAnalysisManager &) {
  errs() << "[Flattening] Pass running on " << F.getName() << "\n";
  errs() << "[Flattening] Original IR:\n";
  F.print(errs());
  if (flatten(&F)) {
    return PreservedAnalyses::none();
  }
  
  return PreservedAnalyses::all();
}

bool Flattening::flatten(Function *f) {

  // SCRAMBLER
  // char scrambling_key[16];
  // get_bytes(scrambling_key, 16);
  // END OF SCRAMBLER

  // 随机数种子
  srand(time(0));

  // 获取函数的所有基本块
  errs() << "[Flattening] Before processing basic blocks\n";
  vector<BasicBlock *> origBB;
  for (BasicBlock &BB : *f) {
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
  entryBB.getTerminator()->eraseFromParent();

  // 创建主循环
  errs() << "[Flattening] Before creating main loop structure\n";
  BasicBlock *loopEntry = BasicBlock::Create(f->getContext(), "loopEntry", f);
  BasicBlock *loopEnd = BasicBlock::Create(f->getContext(), "loopEnd", f);
  BasicBlock *swDefault = BasicBlock::Create(f->getContext(), "switchDefault", f);
  
  // 设置loopEnd块的终止指令
  IRBuilder<> endBuilder(loopEnd);
  endBuilder.CreateBr(loopEntry);

  // 创建调度变量
  IRBuilder<> entryBuilder(&entryBB);
  entryBuilder.CreateBr(loopEntry);
  AllocaInst *switchVar = entryBuilder.CreateAlloca(
    entryBuilder.getInt32Ty(), nullptr, "switchVar");
  [[maybe_unused]] StoreInst *store = entryBuilder.CreateStore(
    entryBuilder.getInt32(0), switchVar);

  // 创建调度块
  IRBuilder<> dispatchBuilder(loopEntry);
  LoadInst *load = dispatchBuilder.CreateLoad(switchVar->getAllocatedType(), switchVar, "switchVar");
  SwitchInst *switchI = dispatchBuilder.CreateSwitch(load, swDefault, origBB.size());
  dispatchBuilder.CreateBr(swDefault);

  // 设置switchDefault块的终止指令
  IRBuilder<> defaultBuilder(swDefault);
  defaultBuilder.CreateBr(loopEnd);

  // 将原始基本块添加到switch指令中
  int randNumCase = rand();
  for (BasicBlock *bb : origBB) {
    bb->moveBefore(loopEnd);
    switchI->addCase(dispatchBuilder.getInt32(randNumCase), bb);
    randNumCase = rand();
  }

  // 修改每个基本块的终止指令
  errs() << "[Flattening] Before modifying terminators\n";
  for (BasicBlock *bb : origBB) {
    if (!bb->getTerminator()) continue;

    if (BranchInst *br = dyn_cast<BranchInst>(bb->getTerminator())) {
      // 如果是条件跳转
      if (br->isConditional()) {
        ConstantInt *numCaseTrue = switchI->findCaseDest(br->getSuccessor(0));
        ConstantInt *numCaseFalse = switchI->findCaseDest(br->getSuccessor(1));

        if (numCaseTrue == nullptr) {
          numCaseTrue = dispatchBuilder.getInt32(randNumCase);
          randNumCase = rand();
        }

        if (numCaseFalse == nullptr) {
          numCaseFalse = dispatchBuilder.getInt32(randNumCase);
          randNumCase = rand();
        }

        // 创建新的条件跳转指令
        Value *cond = br->getCondition();
        BranchInst::Create(bb->getTerminator()->getSuccessor(0), bb->getTerminator()->getSuccessor(1), cond, bb->getTerminator()->getIterator());
        // 创建新的switch case
        switchI->addCase(numCaseTrue, bb->getTerminator()->getSuccessor(0));
        switchI->addCase(numCaseFalse, bb->getTerminator()->getSuccessor(1));
      
      // 如果是无条件跳转
      } else {
        BasicBlock *succ = br->getSuccessor(0);
        br->eraseFromParent();

        ConstantInt *numCase = switchI->findCaseDest(succ);
        if (numCase == nullptr) {
          numCase = dispatchBuilder.getInt32(randNumCase);
          randNumCase = rand();
        }

        // 更新switch变量并跳转到循环结束
        [[maybe_unused]] StoreInst *store = dispatchBuilder.CreateStore(numCase, switchVar);
        BranchInst::Create(loopEnd, bb);
      }
    }
  }

  // 修复栈帧
  errs() << "[Flattening] Final IR before fixStack:\n";
  f->print(errs());
  // fixStack(*f);

  return true;
}

// 修复栈帧
void Flattening::fixStack(Function &F) {
  errs() << "[Flattening] Fixing stack for function: " << F.getName() << "\n";

  vector<PHINode*> origPHI;
  vector<Instruction*> origReg;
  do{
      origPHI.clear();
      origReg.clear();
      BasicBlock &entryBB = F.getEntryBlock();
      for(BasicBlock &BB : F){
          for(Instruction &I : BB){
              if(PHINode *PN = dyn_cast<PHINode>(&I)){
                  origPHI.push_back(PN);
              }else if(!(isa<AllocaInst>(&I) && I.getParent() == &entryBB)
                  && I.isUsedOutsideOfBlock(&BB)){
                  origReg.push_back(&I);
              }
          }
      }

      errs() << "[Flattening] Found " << origPHI.size() << " PHI nodes and " 
      << origReg.size() << " instructions to process\n";

      for(PHINode *PN : origPHI){
          // Skip PHI nodes that are already being processed
          if(PN->getNumIncomingValues() == 0) continue;
          
          // Ensure all incoming blocks and values are valid
          bool valid = true;
          for(unsigned i = 0; i < PN->getNumIncomingValues(); ++i) {
              if(!PN->getIncomingBlock(i) || !PN->getIncomingValue(i)) {
                  errs() << "[Flattening] Warning: Invalid PHI node " << *PN 
                         << " - skipping\n";
                  valid = false;
                  break;
              }
          }
          if(valid) {
              // Additional validation before demoting
              if(PN->getParent() && !PN->getType()->isVoidTy()) {
                  DemotePHIToStack(PN, entryBB.getTerminator()->getIterator());
              } else {
                  errs() << "[Flattening] Warning: Invalid PHI node context " 
                         << *PN << " - skipping\n";
              }
          }
      }
      for(Instruction *I : origReg){
          DemoteRegToStack(*I, false, entryBB.getTerminator()->getIterator());
      }
  }while(!origPHI.empty() || !origReg.empty());

  errs() << "[Flattening] Finished fixing stack for function: " << F.getName() << "\n";
}
