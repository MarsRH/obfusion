#include "OBFS/TestPass1.h"

using namespace llvm;
using namespace OBFS;

PreservedAnalyses TestPass1::run(Function &F, FunctionAnalysisManager &) {
  errs() << "[TestPass1] Found function: " << F.getName() << "\n";
  return PreservedAnalyses::all();
}
