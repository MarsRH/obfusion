#!/bin/bash

# 检查参数数量
if [ $# -lt 1 ]; then
    echo "Usage: $0 <mode> [options]"
    echo "Modes:"
    echo "  build - 构建项目 (cmake & make)"
    echo "  test <input.c> [optimization-level] - 测试PASS插件"
    echo "Example:"
    echo "  $0 build"
    echo "  $0 test haha.c O1"
    exit 1
fi

mode=$1
shift  # 移除模式参数，剩余参数传递给对应函数

case "$mode" in
    build)
        # 构建模式
        echo "Building project..."
        mkdir -p build && cd build && cmake .. && make
        ;;
    test)
        # 测试模式
        if [ $# -lt 1 ]; then
            echo "Test mode requires input file"
            exit 1
        fi
        input_file=$1
        optim_level=${2:-"O1"}  # 默认为O1优化
        
        # 获取不带路径和后缀的文件名
        filename=$(basename "$input_file")
        base_name="${filename%.*}"
        # extension="${filename##*.}"

        # 生成LLVM IR中间表示
        # clang -S -emit-llvm -x $extension "-${optim_level}" "./programs/$input_file" -o "./programs/${base_name}.ll"
        clang -S -emit-llvm "-${optim_level}" "./programs/$input_file" -o "./programs/${base_name}.ll"

        # 调用PASS PLUGIN对目标IR进行优化
        opt -load-pass-plugin ./build/lib/TestPassPlugin.so -passes=test-pass "./programs/${base_name}.ll" -o "./programs/${base_name}.bc"

        echo "Processing completed: ${base_name}.bc generated"
        ;;
    *)
        # 预留其他模式扩展
        echo "Unknown mode: $mode"
        exit 1
        ;;
esac
