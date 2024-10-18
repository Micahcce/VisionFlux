# 设置 build 目录路径
$buildDir = "build"

# 检查目录是否存在，如果存在则删除
if (Test-Path $buildDir) {
    Remove-Item -Recurse -Force $buildDir
}

# 创建新的 build 目录
New-Item -ItemType Directory -Path $buildDir

# 进入 build 目录
Set-Location $buildDir

# 执行 cmake ..
cmake ..

# 执行 MSBuild 编译 MediaManager.sln
MSBuild MediaManager.sln

# 设置目标目录路径
$debugDir = "Debug"
$releaseDir = "Release"
$libsDir = "../../libs"

# 检查 Debug 目录是否存在
if (Test-Path $debugDir) {
    $sourceDir = $debugDir
} elseif (Test-Path $releaseDir) {
    $sourceDir = $releaseDir
} else {
    Write-Host "Neither Debug nor Release directory found."
    exit
}

# 确保目标 libs 目录存在
if (-Not (Test-Path $libsDir)) {
    New-Item -ItemType Directory -Path $libsDir
}

# 复制 .pyd 文件到 ../../libs 目录
$files = Get-ChildItem -Path "$sourceDir\*.pyd"
foreach ($file in $files) {
    Copy-Item -Path $file.FullName -Destination $libsDir -Force
}

# 等待按键，以阻止窗口关闭
Write-Host "Press any key to exit..."
$x = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
