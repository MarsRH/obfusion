#include "llvm/Passes/PassPlugin.h"
#include "OBFS/TestPass.h"
#include "OBFS/TestPass1.h"

using namespace llvm;

// 注册插件（必须导出 C 接口）
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
          if (Name == "test-pass") {
            FPM.addPass(OBFS::TestPass());
            return true;
          }
          if (Name == "test-pass1") {
            FPM.addPass(OBFS::TestPass1());
            return true;
          }
          return false;
        });
    }
  };
}