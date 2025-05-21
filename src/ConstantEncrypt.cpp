
#include "OBFS/ConstantEncrypt.h"
// #include <vector>

// using namespace llvm;
// using namespace OBFS;

// PreservedAnalyses OBFS::ConstantEncrypt::run(Module &M, ModuleAnalysisManager &AM) {
//   errs() << "[ConstantEncrypt] Pass running on " << M.getName() << "\n";
  
//   return PreservedAnalyses::all();
// }

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/SHA1.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"
 
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <vector>
 
#include "llvm/CryptoUtils.h"
 
using namespace llvm;
 

 
namespace OBFS {
 
static cl::opt<int>
    ObfuTimes("gvobfus-times", cl::init(1),
              cl::desc("Run GlobalsEncryption pass <gvobfus-times> time(s)"));
 
static cl::opt<bool> OnlyStr("onlystr", cl::init(false),
                             cl::desc("Encrypt string variable only"));

std::string GenHashedName(GlobalVariable *GV) {
  Module &M = *GV->getParent();
  std::string funcName =
      formatv("{0}_{1:x-}", M.getName(), M.getMDKindID(GV->getName()));
  SHA1 sha1;
  sha1.update(funcName);
  auto digestArray = sha1.final();
  StringRef digest(reinterpret_cast<const char*>(digestArray.data()), digestArray.size());
 
  std::stringstream ss;
  ss << std::hex;
 
  for (size_t i = 0; i < digest.size(); i++) {
    ss << std::setw(2) << std::setfill('0') << (unsigned)(digest[i] & 0xFF);
  }
 
  return ss.str();
}
 
void ConstantEncrypt::InsertIntDecryption(Module &M, EncryptedGV encGV, LLVMContext &ctx) {
  std::vector<Type *> funcArgs;
  FunctionType *funcType =
      FunctionType::get(Type::getVoidTy(M.getContext()), funcArgs, false);
  std::string funcName = GenHashedName(encGV.GV);
  FunctionCallee callee = M.getOrInsertFunction(funcName, funcType);
  Function *func = cast<Function>(callee.getCallee());
 
  BasicBlock *entry = BasicBlock::Create(ctx, "entry", func);
  IRBuilder<> builder(ctx);
  builder.SetInsertPoint(entry);
  LoadInst *val = builder.CreateLoad(encGV.GV->getValueType(), encGV.GV);
  Value *xorVal = builder.CreateXor(
      val, ConstantInt::get(encGV.GV->getValueType(), encGV.key));
  builder.CreateStore(xorVal, encGV.GV);
  builder.CreateRetVoid();
  appendToGlobalCtors(M, func, 0);
}
 
void ConstantEncrypt::InsertArrayDecryption(Module &M, EncryptedGV encGV, LLVMContext &ctx) {
  std::vector<Type *> funcArgs;
  FunctionType *funcType =
      FunctionType::get(Type::getVoidTy(M.getContext()), funcArgs, false);
  std::string funcName = GenHashedName(encGV.GV);
  FunctionCallee callee = M.getOrInsertFunction(funcName, funcType);
  Function *func = cast<Function>(callee.getCallee());
 
  BasicBlock *entry = BasicBlock::Create(ctx, "entry", func);
  BasicBlock *forCond = BasicBlock::Create(ctx, "for.cond", func);
  BasicBlock *forBody = BasicBlock::Create(ctx, "for.body", func);
  BasicBlock *forInc = BasicBlock::Create(ctx, "for.inc", func);
  BasicBlock *forEnd = BasicBlock::Create(ctx, "for.inc", func);
 
  IRBuilder<> builder(ctx);
  Type *Int32Ty = builder.getInt32Ty();
  builder.SetInsertPoint(entry);
  AllocaInst *indexPtr =
      builder.CreateAlloca(Int32Ty, ConstantInt::get(Int32Ty, 1, false), "i");
  builder.CreateStore(ConstantInt::get(Int32Ty, 0), indexPtr);
  builder.CreateBr(forCond);
  builder.SetInsertPoint(forCond);
  LoadInst *index = builder.CreateLoad(Int32Ty, indexPtr);
  ICmpInst *cond = cast<ICmpInst>(
      builder.CreateICmpSLT(index, ConstantInt::get(Int32Ty, encGV.len)));
  builder.CreateCondBr(cond, forBody, forEnd);
  builder.SetInsertPoint(forBody);
  Value *indexList[2] = {ConstantInt::get(Int32Ty, 0), index};
  ArrayType *arrTy = cast<ArrayType>(encGV.GV->getValueType());
  Type *eleTy = arrTy->getElementType();
  Value *ele = builder.CreateGEP(arrTy, encGV.GV, ArrayRef<Value *>(indexList, 2));
  Value *encEle = builder.CreateXor(builder.CreateLoad(eleTy, ele),
                                    ConstantInt::get(eleTy, encGV.key));
  builder.CreateStore(encEle, ele);
  builder.CreateBr(forInc);
  builder.SetInsertPoint(forInc);
  builder.CreateStore(builder.CreateAdd(index, ConstantInt::get(Int32Ty, 1)),
                      indexPtr);
  builder.CreateBr(forCond);
 
  builder.SetInsertPoint(forEnd);
  builder.CreateRetVoid();
  appendToGlobalCtors(M, func, 0);
}
 
PreservedAnalyses ConstantEncrypt::run(Module &M, ModuleAnalysisManager &AM) {
  outs() << "Pass start...\n";
 
  LLVMContext &ctx = M.getContext();
  std::vector<GlobalVariable *> GVs;
 
  for (auto &GV : M.globals()) {
    GVs.push_back(&GV);
  }
 
  for (int i = 0; i < ObfuTimes; i++) {
    outs() << "Current ObfuTimes: " << i << "\n";
    for (auto *GV : GVs) {
      // 只对Integer和Array类型进行加密
      if (!GV->getValueType()->isIntegerTy() &&
          !GV->getValueType()->isArrayTy()) {
        continue;
      }
      // 筛出".str"全局变量，LLVM IR的metadata同样也要保留
      if (GV->hasInitializer() && GV->getInitializer() &&
          (GV->getName().contains(".str") || !OnlyStr) &&
          !GV->getName().contains("llvm.metadata")) {
        Constant *initializer = GV->getInitializer();
        ConstantInt *intData = dyn_cast<ConstantInt>(initializer);
        ConstantDataArray *arrayData = dyn_cast<ConstantDataArray>(initializer);
        // 处理数组
        if (arrayData) {
          // 获取数组的长度和数组元素的大小
          outs() << "Get global arraydata\n";
          uint32_t eleSize = arrayData->getElementByteSize();
          uint32_t eleNum = arrayData->getNumElements();
          uint32_t arrLen = eleNum * eleSize;
          outs() << "Global Variable: " << *GV << "\n"
                 << "Array Length: " << eleSize << " * " << eleNum << " = "
                 << arrLen << "\n";
          char *data = const_cast<char *>(arrayData->getRawDataValues().data());
          char *dataCopy = new char[arrLen];
          memcpy(dataCopy, data, arrLen);
          // 生成密钥
          uint64_t key = cryptoutils->get_uint64_t();
          for (uint32_t i = 0; i < arrLen; i++) {
            dataCopy[i] ^= ((char *)&key)[i % eleSize];
          }
          GV->setInitializer(
              ConstantDataArray::getRaw(StringRef(dataCopy, arrLen), eleNum,
                                        arrayData->getElementType()));
          GV->setConstant(false);
          InsertArrayDecryption(M, {GV, key, eleNum}, ctx);
        }
        // 处理整数
        else if (intData) {
          uint64_t key = cryptoutils->get_uint64_t();
          Constant *enc = ConstantInt::get(intData->getType(),
                                              key ^ intData->getZExtValue());
          GV->setInitializer(enc);
          InsertIntDecryption(M, {GV, key, 1LL}, ctx);
        }
      }
    }
  }
 
  outs() << "Pass end...\n";
 
  return PreservedAnalyses::all();
}
 
} // namespace OBFS
