if(LLVM_OBFUSION_LINK_INTO_TOOLS)
  message(WARNING "Setting LLVM_OBFUSION_LINK_INTO_TOOLS=ON only makes sense for testing purpose")
endif()

if (NOT WIN32 AND NOT CYGWIN)
  # 定义 Pass 插件（关键步骤）
  add_llvm_pass_plugin(TestPass
    TestPass.cpp             # Pass 源文件
    TestPass1.cpp
    Registration.cpp
  #   PLUGIN_NAME "TestPass"   # 插件名称（与注册名称一致）
  #   DEPENDS               # 可选：依赖的 LLVM 表生成文件（如 intrinsics_gen）
  #   LINK_LIBS             # 可选：额外链接库（如 Support）
  )

endif()

# Example
# if (NOT WIN32 AND NOT CYGWIN)
#   add_llvm_pass_plugin(Bye
#     Bye.cpp
#     DEPENDS
#     intrinsics_gen
#     BUILDTREE_ONLY
#    )

#   install(TARGETS ${name} RUNTIME DESTINATION "${LLVM_EXAMPLES_INSTALL_DIR}")
#   set_target_properties(${name} PROPERTIES FOLDER "Examples")
# endif()
