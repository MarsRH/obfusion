#pragma once

#include "OBFS/Common.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/Cloning.h"

namespace OBFS {

class BogusControlFlow : public llvm::PassInfoMixin<BogusControlFlow> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  bool BogusCF(llvm::Function *f);

  llvm::BasicBlock* createCloneBasicBlock(llvm::BasicBlock *BB);

  llvm::Value* createBogusCmp(llvm::IRBuilder<>& builder);

  // 使得该Pass在每次运行时都能被调用
  static bool isRequired() { return true; }
};

}   // namespace OBFS
