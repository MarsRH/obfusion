
#include "OBFS/Constant.h"
#include <vector>

using namespace llvm;
using namespace OBFS;

/**
 * @brief 创建混淆后的字符串全局变量
 * @param M LLVM模块引用
 * @param Str 要混淆的原始字符串
 * @return 返回包含混淆字符串的全局变量指针
 * 
 * 该函数使用简单的异或加密算法(密钥0xAA)对字符串进行混淆，
 * 并将混淆后的字符串存储在模块的全局变量中。
 */
static GlobalVariable* createObfuscatedString(Module &M, StringRef Str) {
  // 使用0xAA作为异或加密密钥
  const char key = 0xAA;
  
  // 对字符串进行逐字符异或加密
  std::vector<char> obfuscated;
  for (char c : Str) {
    obfuscated.push_back(c ^ key);  // 异或加密每个字符
  }
  
  // 创建全局变量来存储加密后的字符串
  auto *GV = new GlobalVariable(
      M, 
      // 创建i8数组类型，长度为加密后字符串长度
      ArrayType::get(Type::getInt8Ty(M.getContext()), obfuscated.size()),
      true,  // 常量标志
      GlobalValue::PrivateLinkage,  // 私有链接，避免被优化掉
      // 创建包含加密数据的常量数组
      ConstantDataArray::get(M.getContext(), 
          ArrayRef<uint8_t>(reinterpret_cast<uint8_t*>(obfuscated.data()), 
                           obfuscated.size())),
      "obfstr");  // 全局变量名称
  
  return GV;
}

/**
 * @brief 常量混淆Pass的主转换函数
 * @param F 要处理的LLVM函数
 * @param AM 函数分析管理器
 * @return 返回保留的分析结果
 * 
 * 该函数遍历函数中的所有调用指令，查找字符串常量参数，
 * 并将其替换为混淆后的版本，同时在运行时动态解密。
 */
PreservedAnalyses OBFS::Constant::run(Function &F, FunctionAnalysisManager &AM) {

  errs() << "[Constant] Pass running on " << F.getName() << "\n";

  Module &M = *F.getParent();  // 获取包含该函数的模块
  
  // 遍历函数中的所有基本块
  for (BasicBlock &BB : F) {
    // 遍历基本块中的所有指令
    for (Instruction &I : BB) {
      // 检查是否是调用指令
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        if (Function *Callee = CI->getCalledFunction()) {
          // 遍历调用指令的所有参数
          for (unsigned i = 0; i < CI->arg_size(); ++i) {
            // 检查参数是否是字符串常量
            if (auto *Str = dyn_cast<ConstantDataSequential>(CI->getArgOperand(i))) {
              if (Str->isString()) {
                // 创建混淆后的字符串全局变量
                GlobalVariable *ObfStr = createObfuscatedString(M, Str->getAsString());
                
                // 创建GEP指令获取字符串首地址
                Value *Idx[] = {
                  ConstantInt::get(Type::getInt32Ty(M.getContext()), 0),  // 数组索引0
                  ConstantInt::get(Type::getInt32Ty(M.getContext()), 0)   // 元素索引0
                };
                Value *GEP = GetElementPtrInst::CreateInBounds(
                    ObfStr->getValueType(), ObfStr, Idx, "", CI);
                
                // 在调用指令前插入解密代码
                IRBuilder<> Builder(CI);
                // 加载加密字符
                Value *Load = Builder.CreateLoad(
                    Type::getInt8Ty(M.getContext()), GEP, "load.str");
                // 使用异或解密字符
                Value *Decrypted = Builder.CreateXor(
                    Load, 
                    ConstantInt::get(Type::getInt8Ty(M.getContext()), 0xAA),
                    "decrypted.str");
                // 替换原始字符串参数为解密后的值
                CI->setArgOperand(i, Decrypted);
              }
            }
          }
        }
      }
    }
  }
  
  // DEBUG: 打印最终的IR
  errs() << "[Constant] Final IR After Constant:\n";
  F.print(errs());

  // 返回保留所有分析结果
  return PreservedAnalyses::all();
}
