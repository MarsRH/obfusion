#pragma once

#include "OBFS/Common.h"

namespace OBFS {

// 字符串加密插件
class ConstantEncrypt : public llvm::PassInfoMixin<ConstantEncrypt> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                             llvm::ModuleAnalysisManager &AM);
  
  static bool isRequired() { return true; }

private:
  // 字符串加密函数
  std::string encryptString(StringRef Str);
  
  // 运行时解密函数
  static char* decryptString(const char* encrypted, size_t length);
};

} // namespace OBFS
