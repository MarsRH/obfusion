#include "OBFS/TestPass.h"

using namespace llvm;
using namespace OBFS;

PreservedAnalyses TestPass::run(Function &F, 
                              FunctionAnalysisManager &AM) {
  // printFunctionName(F); // 使用公共工具函数
  
  // Pass实现逻辑...
  errs() << "[TestPass] Processing\n";
  
  return PreservedAnalyses::all();
}