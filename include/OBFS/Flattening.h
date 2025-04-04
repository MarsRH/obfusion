#pragma once

#include "OBFS/Common.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/LowerSwitch.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Scalar/Reg2Mem.h"

#include <cstdlib> // 包含 rand() 和 srand() 函数
#include <ctime> // 包含 time() 函数

namespace OBFS {

// 控制流平坦化
class Flattening : public llvm::PassInfoMixin<Flattening> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                             llvm::FunctionAnalysisManager &AM);
  
  bool flatten(llvm::Function *f);

  // 使得该Pass在每次运行时都能被调用
  static bool isRequired() { return true; }
};

} // namespace OBFS
