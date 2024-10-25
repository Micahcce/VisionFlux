#!/bin/bash

# 设置 build 目录路径
buildDir="build"

# 检查目录是否存在，如果存在则删除
if [ -d "$buildDir" ]; then
    rm -rf "$buildDir"
fi

# 创建新的 build 目录
mkdir "$buildDir"

# 进入 build 目录
cd "$buildDir" || exit

# 执行 cmake ..
cmake -DENABLE_PYBIND=ON ..

# 执行编译
cmake --build .

# 设置目标目录路径
debugDir="Debug"
releaseDir="Release"
libsDir="../../libs"

# 检查 Debug 或 Release 目录是否存在
if [ -d "$debugDir" ]; then
    sourceDir="$debugDir"
elif [ -d "$releaseDir" ]; then
    sourceDir="$releaseDir"
fi

# 确保目标 libs 目录存在
if [ ! -d "$libsDir" ]; then
    mkdir -p "$libsDir"
fi

# 复制 .so 文件到 ../../libs 目录
for file in "$sourceDir"/*.so; do
    [ -e "$file" ] || continue  # 跳过不存在的文件
    cp -f "$file" "$libsDir"
done

