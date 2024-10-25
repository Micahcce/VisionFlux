#!/bin/bash

# 设置 build 目录路径
buildDir="native_cpp/build"

# 检查目录是否存在，如果存在则删除
if [ -d "$buildDir" ]; then
    rm -rf "$buildDir"
fi

# 创建新的 build 目录
mkdir "$buildDir"

# 进入 build 目录
cd "$buildDir"

# 使用 MinGW 执行 cmake
cmake ..

# 执行编译
cmake --build .