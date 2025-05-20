#include "llvm/Passes/PassPlugin.h"
#include "OBFS/TestPass.h"
#include "OBFS/Flattening.h"
#include "OBFS/BogusControlFlow.h"

using namespace llvm;

// 注册插件（必须导出 C 接口）
// 该函数会在插件加载时被调用 进行插件的注册
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION,
    "ObfusionPass",
    "v1.0",
    [](PassBuilder &PB) {
      PB.registerPipelineParsingCallback(
        [](StringRef Name, FunctionPassManager &FPM,
           ArrayRef<PassBuilder::PipelineElement>) {
          // 测试pass
          if (Name == "test") {
            FPM.addPass(OBFS::TestPass());
            return true;
          }
          // 控制流平坦化
          if (Name == "fla") {
            FPM.addPass(OBFS::Flattening());
            return true;
          }
          // 虚假控制流
          if (Name == "bcf") {
            FPM.addPass(OBFS::BogusControlFlow());
            return true;
          }
          // Other passes go here.
          return false;
        });
    }
  };
}