#include "OBFS/Flattening.h"

using namespace llvm;
using namespace OBFS;

PreservedAnalyses Flattening::run(Function &F, FunctionAnalysisManager &) {
  errs() << "[Flattening] Pass running " << "\n";
  // TODO: Add your code here
  return PreservedAnalyses::all();
}
