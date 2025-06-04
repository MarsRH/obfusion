#pragma once

#include "OBFS/Common.h"

#include "llvm/Support/ManagedStatic.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Value.h"

namespace OBFS {

class OpaquePredicates;
extern llvm::ManagedStatic<OpaquePredicates> opaquepredicate;

class OpaquePredicates {
public:
llvm::Value* generate(llvm::IRBuilder<> &builder);
private:
llvm::Value* templateA(llvm::IRBuilder<> &builder,
    llvm::Module *M,
    llvm::LLVMContext &Context);
llvm::Value* templateB(llvm::IRBuilder<> &builder,
    llvm::Module *M,
    llvm::LLVMContext &Context);
llvm::Value* templateC(llvm::IRBuilder<> &builder,
    llvm::Module *M,
    llvm::LLVMContext &Context);
};

} // namespace OBFS