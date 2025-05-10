#pragma once

#include "OBFS/Common.h"
#include "llvm/IR/Instructions.h"


namespace OBFS {

class BogusCFG : public llvm::PassInfoMixin<BogusCFG> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  bool bogusCFG(llvm::Function *f);

  // 使得该Pass在每次运行时都能被调用
  static bool isRequired() { return true; }
};

}   // namespace OBFS