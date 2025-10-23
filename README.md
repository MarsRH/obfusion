# Obfusion - 基于 LLVM Pass 的代码混淆工具

<div align="center">

![LLVM](https://img.shields.io/badge/LLVM-18+-blue.svg)
![C++](https://img.shields.io/badge/C%2B%2B-17+-orange.svg)
![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg)
![License](https://img.shields.io/badge/License-MIT-yellow.svg)

一个基于 LLVM 的代码混淆工具，提供多种 Pass 插件用于保护代码知识产权和增加逆向分析难度。

</div>

## 📚 Table of Contents

- [✨ Feat](#-feat)
- [🏗️ Architecture](#️-architecture)
- [🚀 QuickStart](#-quickstart)
- [📖 Guide](#-guide)
- [🔧 Development](#-development)
- [🎯 Performance Warning](#-performance-warning)
- [🤝 CONTRIBUTE](#-contribute)
- [📄 LICENSE](#-license)
- [📞 CONTACT](#-contact)

## ✨ Feat

- 🔐 **常量加密** (ConstantEncrypt) - 对全局常量进行加密保护
- 🌀 **控制流平坦化** (Flattening) - 打乱程序控制流结构
- 🎭 **虚假控制流** (BogusControlFlow) - 插入虚假分支增加分析复杂度
- 🔧 **模块化设计** - 基于 LLVM 新 Pass Plugin API
- 🚀 **易于使用** - 简单的命令行工具链

## 🏗️ Architecture

```
obfusion/
├── src/                    # 源代码实现
│   ├── Registration.cpp    # Pass 插件注册入口
│   ├── Flattening.cpp      # 控制流平坦化实现
│   ├── BogusControlFlow.cpp # 虚假控制流实现
│   ├── ConstantEncrypt.cpp # 常量加密实现
│   ├── CryptoUtils.cpp     # 加密工具类
│   └── TestPass.cpp        # 测试 Pass
├── include/OBFS/           # 头文件
│   ├── Common.h            # 公共定义和工具
│   ├── Flattening.h        # 控制流平坦化声明
│   ├── BogusControlFlow.h  # 虚假控制流声明
│   ├── ConstantEncrypt.h   # 常量加密声明
│   └── TestPass.h          # 测试 Pass 声明
├── include/llvm/           # LLVM 相关头文件
├── run.sh                  # 构建和测试脚本
└── CMakeLists.txt          # CMake 构建配置
```

## 🚀 QuickStart

### 系统要求

- LLVM 18+
- CMake 3.20+
- C++17 or Higher
- Linux/macOS (recommend)

### 安装依赖

#### 方式一：使用包管理器安装（推荐）

**Ubuntu/Debian (22.04+)**
```bash
sudo apt update
sudo apt install llvm-18-dev clang-18 cmake build-essential ninja-build
```

**Ubuntu/Debian (较老版本)**
```bash
# 添加 LLVM 官方 APT 仓库
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
echo "deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-18 main" | sudo tee /etc/apt/sources.list.d/llvm.list
sudo apt update
sudo apt install llvm-18-dev clang-18 cmake build-essential ninja-build
```

**CentOS/RHEL/Rocky Linux**
```bash
# 启用 EPEL 仓库
sudo dnf install epel-release
sudo dnf config-manager --add-repo https://apt.llvm.org/llvm.sh
sudo bash llvm.sh 18
sudo dnf install llvm-18-devel clang-18 cmake ninja-build
```

**macOS**
```bash
brew install llvm@18 cmake ninja
```

**Arch Linux**
```bash
sudo pacman -S llvm clang cmake ninja
```

#### 方式二：从源码编译 LLVM（适用于所有发行版）

如果包管理器中的 LLVM 版本不符合要求或需要自定义配置，可以从源码编译：

**1. 安装编译依赖**
```bash
# Ubuntu/Debian
sudo apt install build-essential cmake ninja-build python3-pip git

# CentOS/RHEL
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake ninja-build python3-pip git

# macOS
xcode-select --install
brew install cmake ninja python3
```

**2. 下载并编译 LLVM**
```bash
# 创建工作目录
mkdir -p ~/llvm-build && cd ~/llvm-build

# Clone LLVM 源码
git clone https://github.com/llvm/llvm-project.git
cd llvm-project

# 创建构建目录
mkdir build && cd build

# 配置编译选项
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_ENABLE_RTTI=ON \
  -DLLVM_BUILD_EXAMPLES=ON \
  -DLLVM_TARGETS_TO_BUILD="host" \
  -DCMAKE_INSTALL_PREFIX=$HOME/llvm-install \
  ../llvm

# 编译（编译时间随机器配置决定,可能编译较长时间)
ninja -j$(nproc)

# 安装
ninja install
```

**3. 配置环境变量**
```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
export LLVM_HOME=$HOME/llvm-install
export PATH=$LLVM_HOME/bin:$PATH
export LD_LIBRARY_PATH=$LLVM_HOME/lib:$LD_LIBRARY_PATH
export CMAKE_PREFIX_PATH=$LLVM_HOME/lib/cmake/llvm:$CMAKE_PREFIX_PATH

# 重新加载配置
source ~/.bashrc  # 或 source ~/.zshrc
```

**4. 验证安装**
```bash
llvm-config --version  # 应该显示 18.x.x
clang --version
cmake --version
```

#### 编译参数说明

- `-DCMAKE_BUILD_TYPE=Release`: 发布版本，性能最佳
- `-DLLVM_ENABLE_PROJECTS`: 编译 LLVM 子项目，clang 和 lld 对开发很重要
- `-DLLVM_ENABLE_RTTI=ON`: 启用运行时类型信息，本项目需要
- `-DLLVM_TARGETS_TO_BUILD="host"`: 只编译当前平台的目标，减少编译时间
- `-DCMAKE_INSTALL_PREFIX`: 指定安装路径

### 构建项目

```bash
# 克隆仓库
git clone https://github.com/yourusername/obfusion.git
cd obfusion

# 构建项目
./run.sh build
```

构建成功后会在 `build/lib/` 目录下生成 `libOBFS.so` 插件文件。

## 📖 Guide

### 基本用法

```bash
# 创建测试目录
mkdir -p programs

# 创建测试文件
cat > programs/hello.c << 'EOF'
#include <stdio.h>

int main() {
    for (int i = 0; i < 3; i++) {
        printf("Hello, World! %d\n", i);
    }
    return 0;
}
EOF

# 使用控制流平坦化
./run.sh test hello.c "fla"

# 使用虚假控制流
./run.sh test hello.c "bcf"

# 使用常量加密
./run.sh test hello.c "const"

# 组合使用多个 Pass
./run.sh test hello.c "fla,bcf" O1
```

### Pass 详细说明

#### 1. 控制流平坦化 (`fla`)

将函数的控制流转换为状态机形式，通过一个主循环和 switch 语句来调度所有基本块的执行。

**效果：**
- 破坏原有的控制流结构
- 增加静态分析的难度
- 使控制流图变得复杂

#### 2. 虚假控制流 (`bcf`)

在基本块中插入永远不会执行的虚假分支，使用不透明谓词确保分支结果在运行时总是确定的。

**效果：**
- 增加代码复杂性
- 干扰静态分析工具
- 提高逆向工程难度

#### 3. 常量加密 (`const`)

对全局常量进行加密，在程序运行时通过初始化函数动态解密。

**效果：**
- 隐藏字符串和数值常量
- 防止简单的字符串提取
- 增加反汇编分析的复杂度

### 高级用法

#### 自定义优化级别

```bash
# 使用不同的优化级别
./run.sh test program.c "fla,bcf" O0  # 无优化
./run.sh test program.c "fla,bcf" O1  # 基本优化
./run.sh test program.c "fla,bcf" O2  # 高级优化
./run.sh test program.c "fla,bcf" O3  # 最高优化
```

#### 查看中间表示 (IR)

```bash
# 生成 LLVM IR
clang -S -emit-llvm -O1 programs/hello.c -o programs/hello.ll

# 应用 Pass 并查看结果
opt -S -load-pass-plugin ./build/lib/libOBFS.so -passes="fla" programs/hello.ll -o programs/hello_flattened.ll

# 查看优化后的 IR
cat programs/hello_flattened.ll
```

## 🔧 Development

### 添加新的 Pass

1. 在 `include/OBFS/` 中创建头文件
2. 在 `src/` 中实现 Pass
3. 在 `src/Registration.cpp` 中注册 Pass
4. 在 `src/CMakeLists.txt` 中添加源文件

#### Pass 模板

```cpp
// include/OBFS/MyPass.h
#pragma once

#include "OBFS/Common.h"

namespace OBFS {
class MyPass : public llvm::PassInfoMixin<MyPass> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &AM);

  static bool isRequired() { return true; }
};
}
```

```cpp
// src/MyPass.cpp
#include "OBFS/MyPass.h"

using namespace llvm;
using namespace OBFS;

PreservedAnalyses MyPass::run(Function &F, FunctionAnalysisManager &AM) {
  errs() << "[MyPass] Processing function: " << F.getName() << "\n";

  // 实现混淆逻辑

  return PreservedAnalyses::none(); // 如果修改了 IR
}
```

### 调试技巧

1. **启用调试输出**：在代码中使用 `errs()` 输出调试信息
2. **查看 IR**：使用 `opt -S` 查看优化前后的 LLVM IR
3. **单步调试**：使用 GDB 或 LLDB 调试插件

## 🎯 Performance Warning

- 混淆会增加代码体积和执行时间
- 建议在发布版本中才应用混淆
- 可以根据需要选择性应用不同的 Pass
- 某些 Pass 可能影响编译器优化效果

## 🤝 CONTRIBUTE

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

## 📄 LICENSE

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。


## 📞 CONTACT

- 项目主页：https://github.com/MarsRH/obfusion
- 问题反馈：https://github.com/MarsRH/obfusion/issues
- 邮箱：your.email@example.com

---

<div align="center">

**如果这个项目对你有帮助，请给个 ⭐️ Star！**

</div>