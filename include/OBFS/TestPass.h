#pragma once

#include "OBFS/Common.h"

namespace OBFS {

// 测试插件
class TestPass : public llvm::PassInfoMixin<TestPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                             llvm::FunctionAnalysisManager &AM);
  
  // 如果不需要，可以省略isRequired方法
//   static bool isRequired() { return true; }
};

} // namespace OBFS