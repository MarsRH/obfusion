#pragma once

#include "OBFS/Common.h"

namespace OBFS {

// 测试插件
class Constant : public llvm::PassInfoMixin<Constant> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                             llvm::FunctionAnalysisManager &AM);
  
  static bool isRequired() { return true; }
};

} // namespace OBFS