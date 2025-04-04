cmake_minimum_required(VERSION 3.20)  # LLVM 18+ 要求 CMake 3.20+
project(OBFUSION)

# 查找 LLVM 并引入必要配置
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Using LLVM version: ${LLVM_PACKAGE_VERSION}")

# 设置 LLVM 的 CMake 模块路径
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)           # 引入 LLVM 的 CMake 辅助函数
include(HandleLLVMOptions) # 自动处理 LLVM 编译选项

# 设置头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Use LLVM's preprocessor definitions, include directories, and library search
# paths.
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# 设置库文件输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(src)
