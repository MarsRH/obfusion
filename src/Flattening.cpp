#include "OBFS/Flattening.h"
#include "llvm/CryptoUtils.h"

using namespace llvm;
using namespace OBFS;
using std::vector;

#define DEBUG_TYPE "Flattening"

PreservedAnalyses Flattening::run(Function &F, FunctionAnalysisManager &AM) {
  outs() << "\n" << "[Flattening] Pass running on " << F.getName() << "\n";
  // DEBUG: 打印原始IR
  DEBUG_WITH_TYPE(DEBUG_TYPE, 
    dbgs() << "[Flattening] Original IR:\n"; 
    F.print(dbgs()););

  // 模块声明
  RegToMemPass *reg = new RegToMemPass();
  LowerSwitchPass *lower = new LowerSwitchPass();

  // LowerSwitch
  lower->run(F, AM);
  if (flatten(&F)) {
    // Remove phi nodes
    reg->run(F, AM);   
    return PreservedAnalyses::none();
  }
  
  return PreservedAnalyses::all();
}

bool Flattening::flatten(Function *f) {

  int randNumCase = cryptoutils->get_uint32_t();

  // 获取函数的所有基本块
  vector<BasicBlock *> origBB;
  for (BasicBlock &BB : *f) {
    origBB.emplace_back(&BB);
    // 跳过包含调用指令的基本块
    if (isa<InvokeInst>(BB.getTerminator())) {
      return false;
    }
  }
  outs() << "[Flattening] Collected " << origBB.size() << " basic blocks\n";

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
  // dbgs() << "[Flattening] Creating main loop structure\n";
  BasicBlock *loopEntry = BasicBlock::Create(f->getContext(), "loopEntry", f);
  BasicBlock *loopEnd = BasicBlock::Create(f->getContext(), "loopEnd", f);
  BasicBlock *swDefault = BasicBlock::Create(f->getContext(), "switchDefault", f);


  // 创建调度变量
  IRBuilder<> entryBuilder(&entryBB);
  AllocaInst *switchVar = entryBuilder.CreateAlloca(
    entryBuilder.getInt32Ty(), nullptr, "switchVar");
  entryBuilder.CreateStore(entryBuilder.getInt32(randNumCase), switchVar);
  entryBuilder.CreateBr(loopEntry);

  // 创建调度块
  IRBuilder<> dispatchBuilder(loopEntry);
  LoadInst *load = dispatchBuilder.CreateLoad(switchVar->getAllocatedType(), switchVar, "switchVar");
  SwitchInst *switchI = dispatchBuilder.CreateSwitch(load, swDefault, origBB.size());
  // dispatchBuilder.CreateBr(swDefault);

  // 设置switchDefault块的终止指令
  IRBuilder<> defaultBuilder(swDefault);
  defaultBuilder.CreateBr(loopEnd);  
  // 设置loopEnd块的终止指令
  IRBuilder<> endBuilder(loopEnd);
  endBuilder.CreateBr(loopEntry);

  // 将原始基本块添加到switch指令中
  for (BasicBlock *bb : origBB) {
    bb->moveBefore(loopEnd);
    switchI->addCase(dispatchBuilder.getInt32(randNumCase), bb);
    randNumCase = cryptoutils->get_uint32_t();
  }

  // 修改每个基本块的终止指令
  // dbgs() << "[Flattening] Modifying terminators\n";
  for (BasicBlock *bb : origBB) {
    if (!bb->getTerminator()) continue;

    if (BranchInst *br = dyn_cast<BranchInst>(bb->getTerminator())) {
      // 如果是条件跳转
      if (br->isConditional()) {
        ConstantInt *numCaseTrue = switchI->findCaseDest(br->getSuccessor(0));
        ConstantInt *numCaseFalse = switchI->findCaseDest(br->getSuccessor(1));

        if (numCaseTrue == nullptr) {
          numCaseTrue = dispatchBuilder.getInt32(randNumCase);
          randNumCase = cryptoutils->get_uint32_t();
        }

        if (numCaseFalse == nullptr) {
          numCaseFalse = dispatchBuilder.getInt32(randNumCase);
          randNumCase = cryptoutils->get_uint32_t();
        }

        
        // 创建新的条件跳转指令
        IRBuilder<> caseBuilder(bb);
        Value *cond = caseBuilder.CreateSelect(
          br->getCondition(), numCaseTrue, numCaseFalse, "cond" );
        caseBuilder.CreateStore(cond, switchVar);
        caseBuilder.CreateBr(loopEnd);
        br->eraseFromParent();
      
      // 如果是无条件跳转
      } else {
        BasicBlock *succ = br->getSuccessor(0);
        br->eraseFromParent();

        // 创建新的无条件跳转指令
        ConstantInt *numCase = switchI->findCaseDest(succ);
        if (numCase == nullptr) {
          numCase = dispatchBuilder.getInt32(randNumCase);
          randNumCase = cryptoutils->get_uint32_t();
        }

        // 更新switch变量并跳转到循环结束
        IRBuilder<> caseBuilder(bb);
        caseBuilder.CreateStore(numCase, switchVar);
        caseBuilder.CreateBr(loopEnd);
      }
    }
  }

  // DEBUG: 打印最终的IR
  DEBUG_WITH_TYPE(DEBUG_TYPE, 
    dbgs() << "[Flattening] Final IR After Flattening:\n"; 
    f->print(dbgs()););
  
  outs() << "[Flattening] Flattening Done\n";

  return true;
}

Flattening::Flattening() {
  outs() << "[Flattening] Initialized" << "\n";
}
