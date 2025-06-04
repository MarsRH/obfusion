// 防止头文件重复包含
#pragma once

// #include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/ErrorHandling.h"

#include <vector>
#include <cctype>

// 公共命名空间
namespace OBFS {

// 公共工具函数或共享定义
inline void printFunctionName(llvm::Function &F) {
  llvm::errs() << "Processing function: " << F.getName() << "\n";
}

} // namespace OBFS
