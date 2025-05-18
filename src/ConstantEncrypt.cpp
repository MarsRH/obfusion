
#include "OBFS/ConstantEncrypt.h"
#include <vector>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/GlobalVariable.h>

using namespace llvm;
using namespace OBFS;

// 字符串加密函数
std::string ConstantEncrypt::encryptString(StringRef Str) {
  std::string encrypted;
  const char* data = Str.data();
  size_t length = Str.size();
  
  // 简单XOR加密
  for (size_t i = 0; i < length; ++i) {
    encrypted.push_back(data[i] ^ 0xAA);
  }
  return encrypted;
}

// 运行时解密函数
char* ConstantEncrypt::decryptString(const char* encrypted, size_t length) {
  char* decrypted = new char[length + 1];
  for (size_t i = 0; i < length; ++i) {
    decrypted[i] = encrypted[i] ^ 0xAA;
  }
  decrypted[length] = '\0';
  return decrypted;
}

PreservedAnalyses OBFS::ConstantEncrypt::run(Module &M, ModuleAnalysisManager &AM) {
  errs() << "[ConstantEncrypt] Pass running on " << M.getName() << "\n";
  
  // 创建并实现解密函数
  FunctionType *DecryptFuncType = FunctionType::get(
      Type::getInt8PtrTy(M.getContext()),
      {Type::getInt8PtrTy(M.getContext()), Type::getInt64Ty(M.getContext())},
      false);
  Function *DecryptFunc = Function::Create(DecryptFuncType, 
                                        GlobalValue::ExternalLinkage,
                                        "OBFS_DecryptString", &M);
  
  // 添加函数实现
  BasicBlock *BB = BasicBlock::Create(M.getContext(), "entry", DecryptFunc);
  IRBuilder<> Builder(BB);
  
  // 获取函数参数
  Value *EncryptedArg = DecryptFunc->arg_begin();
  Value *LengthArg = DecryptFunc->arg_begin() + 1;
  
  // 调用静态解密方法
  Value *DecryptedPtr = Builder.CreateCall(
      ConstantEncrypt::decryptString,
      {EncryptedArg, LengthArg});
  
  // 返回解密后的字符串
  Builder.CreateRet(DecryptedPtr);

  // 遍历模块中的所有全局变量
  for (GlobalVariable &GV : M.globals()) {
    if (GV.isConstant() && GV.hasInitializer()) {
      Constant *Init = GV.getInitializer();
      
      // 检查是否是字符串常量
      if (ConstantDataSequential *CDS = dyn_cast<ConstantDataSequential>(Init)) {
        if (CDS->isString()) {
          StringRef Str = CDS->getAsString();
          std::string encrypted = encryptString(Str);
          
          // 创建加密后的全局变量
          Constant *NewInit = ConstantDataArray::getString(
              M.getContext(), encrypted, false);
          GV.setInitializer(NewInit);
          
          errs() << "Encrypted string: " << Str << " -> " << encrypted << "\n";
        }
      }
    }
  }

  return PreservedAnalyses::all();
}
