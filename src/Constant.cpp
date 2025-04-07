#include "OBFS/Constant.h"
#include <vector>

using namespace llvm;
using namespace OBFS;

// 字符串混淆函数
static GlobalVariable* createObfuscatedString(Module &M, StringRef Str) {
  // 异或密钥
  const char key = 0xAA;
  
  // 加密字符串
  std::vector<char> obfuscated;
  for (char c : Str) {
    obfuscated.push_back(c ^ key);
  }
  
  // 创建全局变量存储加密字符串
  auto *GV = new GlobalVariable(
      M, 
      ArrayType::get(Type::getInt8Ty(M.getContext()), obfuscated.size()),
      true,
      GlobalValue::PrivateLinkage,
      ConstantDataArray::get(M.getContext(), 
          ArrayRef<uint8_t>(reinterpret_cast<uint8_t*>(obfuscated.data()), 
                           obfuscated.size())),
      "obfstr");
  
  return GV;
}

PreservedAnalyses Constant::run(Function &F, FunctionAnalysisManager &AM) {
  Module &M = *F.getParent();
  
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (Function *Callee = CI->getCalledFunction()) {
          // 处理字符串参数
          for (unsigned i = 0; i < CI->arg_size(); ++i) {
            if (auto *Str = dyn_cast<ConstantDataSequential>(CI->getArgOperand(i))) {
              if (Str->isString()) {
                // 创建混淆字符串并替换原参数
                GlobalVariable *ObfStr = createObfuscatedString(M, Str->getAsString());
                Value *Idx[] = {
                  ConstantInt::get(Type::getInt32Ty(M.getContext()), 0),
                  ConstantInt::get(Type::getInt32Ty(M.getContext()), 0)
                };
                Value *GEP = GetElementPtrInst::CreateInBounds(
                    ObfStr->getValueType(), ObfStr, Idx, "", CI);
                
                // 插入解密代码
                IRBuilder<> Builder(CI);
                Value *Load = Builder.CreateLoad(
                    Type::getInt8Ty(M.getContext()), GEP, "load.str");
                Value *Decrypted = Builder.CreateXor(
                    Load, 
                    ConstantInt::get(Type::getInt8Ty(M.getContext()), 0xAA),
                    "decrypted.str");
                CI->setArgOperand(i, Decrypted);
              }
            }
          }
        }
      }
    }
  }
  
  return PreservedAnalyses::all();
}
