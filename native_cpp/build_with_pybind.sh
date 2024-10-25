#!/bin/bash

# ���� build Ŀ¼·��
buildDir="build"

# ���Ŀ¼�Ƿ���ڣ����������ɾ��
if [ -d "$buildDir" ]; then
    rm -rf "$buildDir"
fi

# �����µ� build Ŀ¼
mkdir "$buildDir"

# ���� build Ŀ¼
cd "$buildDir" || exit

# ִ�� cmake ..
cmake -DENABLE_PYBIND=ON ..

# ִ�б���
cmake --build .

# ����Ŀ��Ŀ¼·��
debugDir="Debug"
releaseDir="Release"
libsDir="../../libs"

# ��� Debug �� Release Ŀ¼�Ƿ����
if [ -d "$debugDir" ]; then
    sourceDir="$debugDir"
elif [ -d "$releaseDir" ]; then
    sourceDir="$releaseDir"
fi

# ȷ��Ŀ�� libs Ŀ¼����
if [ ! -d "$libsDir" ]; then
    mkdir -p "$libsDir"
fi

# ���� .so �ļ��� ../../libs Ŀ¼
for file in "$sourceDir"/*.so; do
    [ -e "$file" ] || continue  # ���������ڵ��ļ�
    cp -f "$file" "$libsDir"
done

