if(LLVM_OBFUSION_LINK_INTO_TOOLS)
  message(WARNING "Setting LLVM_OBFUSION_LINK_INTO_TOOLS=ON only makes sense for testing purpose")
endif()

if (NOT WIN32 AND NOT CYGWIN)
  # 定义 Pass 插件（关键步骤）
  add_llvm_pass_plugin(libOBFS
    CryptoUtils.cpp          # 加密工具类
    ConstantEncrypt.cpp      # Constant 源文件
    TestPass.cpp             # Pass 源文件
    Flattening.cpp           # Flattening 源文件
    Registration.cpp         # 注册文件
  )

endif()