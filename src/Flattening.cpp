#include "OBFS/Flattening.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

using namespace llvm;
using namespace OBFS;

PreservedAnalyses Flattening::run(Function &F, FunctionAnalysisManager &) {
  errs() << "[Flattening] Pass running on " << F.getName() << "\n";
  return PreservedAnalyses::all();
}

bool Flattening::flatten(Function *f) {

  // SCRAMBLER
  char scrambling_key[16];
  get_bytes(scrambling_key, 16);
  // END OF SCRAMBLER

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

  // 将原始基本块添加到switch指令中
  for (BasicBlock *bb : origBB) {
    bb->moveBefore(loopEnd);
    switchI->addCase(dispatchBuilder.getInt32(/**TODO: Add nums*/), bb);
  }

  // 修改每个基本块的终止指令
  for (BasicBlock *bb : origBB) {
    if (!bb->getTerminator()) continue;

    if (BranchInst *br = dyn_cast<BranchInst>(bb->getTerminator())) {
      // 如果是条件跳转
      if (br->isConditional()) {
        ConstantInt *numCaseTrue = switchI->findCaseDest(br->getSuccessor(0));
        ConstantInt *numCaseFalse = switchI->findCaseDest(br->getSuccessor(1));

        if (numCaseTrue == nullptr) {
          numCaseTrue = dispatchBuilder.getInt32(/**TODO: Add nums*/);
        }

        if (numCaseFalse == nullptr) {
          numCaseFalse = dispatchBuilder.getInt32(/**TODO: Add nums*/);
        }

        // 创建新的条件跳转指令
        Value *cond = br->getCondition();
        BranchInst::Create(bb->getTerminator()->getSuccessor(0), bb->getTerminator()->getSuccessor(1), cond, bb->getTerminator());
        // 创建新的switch case
        switchI->addCase(numCaseTrue, bb->getTerminator()->getSuccessor(0));
        switchI->addCase(numCaseFalse, bb->getTerminator()->getSuccessor(1));
      
      // 如果是无条件跳转
      } else {
        BasicBlock *succ = br->getSuccessor(0);
        br->eraseFromParent();

        ConstantInt *numCase = switchI->findCaseDest(succ);
        if (numCase == nullptr) {
          numCase = dispatchBuilder.getInt32(/**TODO: Add nums*/);
        }

        // 更新switch变量并跳转到循环结束
        store = dispatchBuilder.CreateStore(numCase, switchVar);
        BranchInst::Create(loopEnd, bb);
      }
    }
  }

  // 修复栈帧
  fixStack(f);

  return true;
}

// TODO: CODE REVIEW

// 修复栈帧
void Flattening::fixStack(Function &F) {
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
      for(PHINode *PN : origPHI){
          DemotePHIToStack(PN, entryBB.getTerminator());
      }
      for(Instruction *I : origReg){
          DemoteRegToStack(*I, entryBB.getTerminator());
      }
  }while(!origPHI.empty() || !origReg.empty());
}

// 生成随机数
unsigned Flattening::scramble32(const unsigned in, const char key[16]) {
  assert(key != NULL && "CryptoUtils::scramble key=NULL");

  unsigned tmpA, tmpB;

  // Orr, Nathan or Adi can probably break it, but who cares?

  // Round 1
  tmpA = 0x0;
  tmpA ^= AES_PRECOMP_TE0[((in >> 24) ^ key[0]) & 0xFF];
  tmpA ^= AES_PRECOMP_TE1[((in >> 16) ^ key[1]) & 0xFF];
  tmpA ^= AES_PRECOMP_TE2[((in >> 8) ^ key[2]) & 0xFF];
  tmpA ^= AES_PRECOMP_TE3[((in >> 0) ^ key[3]) & 0xFF];

  // Round 2
  tmpB = 0x0;
  tmpB ^= AES_PRECOMP_TE0[((tmpA >> 24) ^ key[4]) & 0xFF];
  tmpB ^= AES_PRECOMP_TE1[((tmpA >> 16) ^ key[5]) & 0xFF];
  tmpB ^= AES_PRECOMP_TE2[((tmpA >> 8) ^ key[6]) & 0xFF];
  tmpB ^= AES_PRECOMP_TE3[((tmpA >> 0) ^ key[7]) & 0xFF];

  // Round 3
  tmpA = 0x0;
  tmpA ^= AES_PRECOMP_TE0[((tmpB >> 24) ^ key[8]) & 0xFF];
  tmpA ^= AES_PRECOMP_TE1[((tmpB >> 16) ^ key[9]) & 0xFF];
  tmpA ^= AES_PRECOMP_TE2[((tmpB >> 8) ^ key[10]) & 0xFF];
  tmpA ^= AES_PRECOMP_TE3[((tmpB >> 0) ^ key[11]) & 0xFF];

  // Round 4
  tmpB = 0x0;
  tmpB ^= AES_PRECOMP_TE0[((tmpA >> 24) ^ key[12]) & 0xFF];
  tmpB ^= AES_PRECOMP_TE1[((tmpA >> 16) ^ key[13]) & 0xFF];
  tmpB ^= AES_PRECOMP_TE2[((tmpA >> 8) ^ key[14]) & 0xFF];
  tmpB ^= AES_PRECOMP_TE3[((tmpA >> 0) ^ key[15]) & 0xFF];

  LOAD32H(tmpA, key);

  return tmpA ^ tmpB;
}

void Flattening::get_bytes(char *buffer, const int len) {

  int sofar = 0, available = 0;

  assert(buffer != NULL && "CryptoUtils::get_bytes buffer=NULL");
  assert(len > 0 && "CryptoUtils::get_bytes len <= 0");

  statsGetBytes++;

  if (len > 0) {

    // If the PRNG is not seeded, it the very last time to do it !
    if (!seeded) {
      prng_seed();
      populate_pool();
    }

    do {
      if (idx + (len - sofar) >= CryptoUtils_POOL_SIZE) {
        // We don't have enough bytes ready in the pool,
        // so let's use the available ones and repopulate !
        available = CryptoUtils_POOL_SIZE - idx;
        memcpy(buffer + sofar, pool + idx, available);
        sofar += available;
        populate_pool();
      } else {
        memcpy(buffer + sofar, pool + idx, len - sofar);
        idx += len - sofar;
        // This will trigger a loop exit
        sofar = len;
      }
    } while (sofar < (len - 1));
  }
}