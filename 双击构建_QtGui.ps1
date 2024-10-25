# 设置 build 目录路径
$buildDir = "native_cpp/build"

# 检查目录是否存在，如果存在则删除
if (Test-Path $buildDir) {
    Remove-Item -Recurse -Force $buildDir
}

# 创建新的 build 目录
New-Item -ItemType Directory -Path $buildDir

# 进入 build 目录
Set-Location $buildDir

# 使用 MinGW 执行 cmake
cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..

# 执行编译
cmake --build .

# 等待按键，以阻止窗口关闭
Write-Host "Press any key to exit..."
$x = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")