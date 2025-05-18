#pragma once

#include "OBFS/Common.h"

struct EncryptedGV {
  llvm::GlobalVariable *GV;
  uint64_t key;
  uint32_t len;
};

namespace OBFS {

// 字符串加密插件
class ConstantEncrypt : public llvm::PassInfoMixin<ConstantEncrypt> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M,
                             llvm::ModuleAnalysisManager &AM);

  virtual void InsertIntDecryption(llvm::Module &M, EncryptedGV encGV, llvm::LLVMContext &ctx);
  virtual void InsertArrayDecryption(llvm::Module &M, EncryptedGV encGV, llvm::LLVMContext &ctx);

  static bool isRequired() { return true; }
};

} // namespace OBFS
