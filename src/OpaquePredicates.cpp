#include "OBFS/OpaquePredicates.h"
#include "llvm/CryptoUtils.h"

using namespace llvm;
using namespace OBFS;

#define DEBUG_TYPE "OpaquePredicates"

namespace OBFS {
    ManagedStatic<OpaquePredicates> opaquepredicate;
}

Value* OpaquePredicates::generate(IRBuilder<> &builder) {
    
    Module *M = builder.GetInsertBlock()->getModule();
    LLVMContext &Context = M->getContext();

    int rand = cryptoutils->get_uint32_t();
    if (rand % 3 == 0) {
        return templateA(builder, M, Context);
    } else if (rand % 3 == 1) {
        return templateB(builder, M, Context);
    } else {
        return templateC(builder, M, Context);
    }
}

Value* OpaquePredicates::templateA(IRBuilder<> &builder, Module *M, LLVMContext &Context) {
    // if((y < 10 || x * (x + 1) % 2 == 0))
    // 等价于 if(true)
    
    uint32_t x_val = cryptoutils->get_uint32_t() % 46000;
    uint32_t y_val = cryptoutils->get_uint32_t() % 46000;
    std::string x_name = "var_x_" + std::to_string(cryptoutils->get_uint32_t());
    std::string y_name = "var_y_" + std::to_string(cryptoutils->get_uint32_t());
    GlobalVariable *xptr = new GlobalVariable(*M, Type::getInt32Ty(Context), false, 
        GlobalValue::ExternalLinkage, ConstantInt::get(Type::getInt32Ty(Context), x_val), x_name.c_str());
    GlobalVariable *yptr = new GlobalVariable(*M, Type::getInt32Ty(Context), false, 
        GlobalValue::ExternalLinkage, ConstantInt::get(Type::getInt32Ty(Context), y_val), y_name.c_str());
    
    Value *x = builder.CreateLoad(Type::getInt32Ty(Context), xptr);
    Value *y = builder.CreateLoad(Type::getInt32Ty(Context), yptr);
    Value *cond1 = builder.CreateICmpSLT(y, ConstantInt::get(Type::getInt32Ty(Context), 10));
    Value *op1 = builder.CreateAdd(x, ConstantInt::get(Type::getInt32Ty(Context), 1));
    Value *op2 = builder.CreateMul(op1, x);
    Value *op3 = builder.CreateURem(op2, ConstantInt::get(Type::getInt32Ty(Context), 2));
    Value *cond2 = builder.CreateICmpEQ(op3, ConstantInt::get(Type::getInt32Ty(Context), 0));
    outs() << "Template A" << "\n";
    return builder.CreateOr(cond1, cond2);
}

Value* OpaquePredicates::templateB(IRBuilder<> &builder, Module *M, LLVMContext &Context) {
    
    uint32_t x_val = cryptoutils->get_uint32_t() % 46000;
    std::string x_name = "var_xx_" + std::to_string(cryptoutils->get_uint32_t());
    GlobalVariable *xptr = new GlobalVariable(*M, Type::getInt32Ty(Context), false,
        GlobalValue::ExternalLinkage, ConstantInt::get(Type::getInt32Ty(Context), x_val), x_name.c_str());
    
    Value *x = builder.CreateLoad(Type::getInt32Ty(Context), xptr);
    
    // 左边表达式: x*x + 2*x + 1
    Value *left1 = builder.CreateMul(x, x);
    Value *left2 = builder.CreateMul(ConstantInt::get(Type::getInt32Ty(Context), 2), x);
    Value *left3 = builder.CreateAdd(left1, left2);
    Value *left = builder.CreateAdd(left3, ConstantInt::get(Type::getInt32Ty(Context), 1));
    
    // 右边表达式: (x + 1)*(x + 1)
    Value *right1 = builder.CreateAdd(x, ConstantInt::get(Type::getInt32Ty(Context), 1));
    Value *right = builder.CreateMul(right1, right1);

    outs() << "Template B" << "\n";
    // 比较两边是否相等
    return builder.CreateICmpEQ(left, right);
}

Value* OpaquePredicates::templateC(IRBuilder<> &builder, Module *M, LLVMContext &Context) {
    
    // 创建全局变量a和b
    uint32_t a_val = cryptoutils->get_uint32_t() % 46000;
    uint32_t b_val = cryptoutils->get_uint32_t() % 46000;
    std::string a_name = "var_a_" + std::to_string(cryptoutils->get_uint32_t());
    std::string b_name = "var_b_" + std::to_string(cryptoutils->get_uint32_t());
    GlobalVariable *aptr = new GlobalVariable(*M, Type::getInt32Ty(Context), false,
        GlobalValue::ExternalLinkage, ConstantInt::get(Type::getInt32Ty(Context), a_val), a_name.c_str());
    GlobalVariable *bptr = new GlobalVariable(*M, Type::getInt32Ty(Context), false,
        GlobalValue::ExternalLinkage, ConstantInt::get(Type::getInt32Ty(Context), b_val), b_name.c_str());
    
    Value *a = builder.CreateLoad(Type::getInt32Ty(Context), aptr);
    Value *b = builder.CreateLoad(Type::getInt32Ty(Context), bptr);
    
    // 模数m设为随机数
    uint32_t m_val = cryptoutils->get_uint32_t() % 1145 + 1; // 确保m不为0
    Value *m = ConstantInt::get(Type::getInt32Ty(Context), m_val);
    
    // 计算左边表达式: (a*b) mod m
    Value *left1 = builder.CreateMul(a, b);
    Value *left = builder.CreateURem(left1, m);
    
    // 计算右边表达式: [(a mod m)*(b mod m)] mod m
    Value *right1 = builder.CreateURem(a, m);
    Value *right2 = builder.CreateURem(b, m);
    Value *right3 = builder.CreateMul(right1, right2);
    Value *right = builder.CreateURem(right3, m);
    
    outs() << "Template C" << "\n";
    // 比较两边是否相等
    return builder.CreateICmpEQ(left, right);
}
