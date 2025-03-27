// 防止头文件重复包含
#pragma once

// #include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"

// 公共命名空间
namespace OBFS {

// 公共工具函数或共享定义
inline void printFunctionName(llvm::Function &F) {
  llvm::errs() << "Processing function: " << F.getName() << "\n";
}

#define LOAD32H(x, y)                                                          \
  {                                                                            \
    (x) = ((uint32_t)((y)[0] & 0xFF) << 24) |                                  \
          ((uint32_t)((y)[1] & 0xFF) << 16) |                                  \
          ((uint32_t)((y)[2] & 0xFF) << 8) | ((uint32_t)((y)[3] & 0xFF) << 0); \
  }


} // namespace OBFS
