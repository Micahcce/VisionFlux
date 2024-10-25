# VisionFlux：通用多媒体播放器

VisionFlux 是一个基于 FFmpeg 和 SDL 实现的多功能媒体播放器项目。项目设计采用 C++ 编写的底层媒体处理逻辑，并使用 Qt 实现图形界面（GUI）。通过 Pybind11，将播放器功能编译为 Python 模块，方便未来加入 AI 功能（待开发）。

## 功能列表

- **视频播放**：支持主流视频格式的流畅播放。
- **变速播放**：可调整播放速度。
- **暂停与恢复**：控制播放状态。
- **视频切换**：无缝切换不同视频文件。
- **进度条拖动**：支持通过进度条快速跳转视频。
- **音量控制**：调节播放音量。
- **单流播放**：支持仅包含视频流或音频流的文件。
- **实时缩放**：实时调整视频画面大小。
- **帧缓冲队列**：使用帧缓冲机制，保证视频播放的流畅性。
- **音视频同步**：确保音频与视频播放保持同步。
- **硬件加速**：利用硬件资源加速视频解码。
- **流信息查询**：查询媒体文件的音视频流信息。
- **编码与格式转换**：支持不同格式的编码和文件转换。
- **网络拉流**：支持播放网络流，并将其保存。

## 技术栈

- **编程语言**：C++、Python
- **框架**：Qt (用于图形界面开发)
- **主要依赖库**：
  - **FFmpeg**：用于媒体解码与编码。
  - **SDL**：用于音频播放。
  - **SoundTouch**：用于音频的变速与音调调整。
  - **Pybind11**：将 C++ 函数暴露给 Python。
- **工具**：
  - **Qt Creator**：开发环境。
  - **CMake**：构建系统。
  - **多线程**：实现高效的媒体解码与渲染。

## 项目结构

- **底层逻辑**：使用纯 C++ 编写，负责媒体解码、编码、播放控制等。底层使用 FFmpeg 和 SDL 处理媒体数据，支持视频帧缓冲、音视频同步、硬件加速、流管理等功能。
- **前端界面**：采用 Qt 实现的图形界面，提供现代化的播放控制界面，包括播放、暂停、进度调节、音量控制等功能。视频帧通过回调函数渲染到 GUI 上，使用 QImage 来显示。
- **Python 集成**：底层通过 Pybind11 编译为 Python 模块（`.pyd` 文件），可以在 Python 环境中导入模块并进一步开发 AI 功能（目前尚未开发）。

## 安装步骤

### 环境依赖

- FFmpeg (>= 6.x)
- SDL2
- SoundTouch
- Qt (>= 5.x)
- CMake (>= 3.x)
- Python (>= 3.6) 及 Pybind11（可选，用于 Python 集成）

### 环境安装（Linux）

1. Qt工具链

```
sudo apt install qtbase5-dev qtchooser qt5-qmake qttools5-dev-tools
```

2. SDL2

```
sudo apt install libsdl2-2.0-0 libsdl2-dev
```

3. FFmpeg6.0（源码安装）

```
cd ~
wget https://ffmpeg.org/releases/ffmpeg-6.0.tar.bz2
tar xjf ffmpeg-6.0.tar.bz2
cd ffmpeg-6.0

./configure --enable-gpl --enable-libx264 --enable-libx265 --enable-libmp3lame \
--enable-libvorbis --enable-libass --enable-libfreetype --enable-libopus \
--enable-nonfree --enable-shared

make -j$(nproc)  	# 使用所有可用的处理器核心加速编译
sudo make install
sudo ldconfig		# 更新共享库
```

### 注意事项

* 当编译时出现 `error C2059: 语法错误:“)”`或类似错误，将错误中涉及的文件以 UTF-8 BOM 编码格式保存后，重新编译即可。
* 若希望构建为 Python 模块，Winodws下请使用 MSVC 编译此项目，与 Python 兼容。另注意编译使用的 Python 与集成模块使用的 Python 版本需要一致。
* 当开启或关闭 `ENABLE_PYBIND` 不生效时，请手动设置 `CMakeList.txt` 文件中 `set(ENABLE_PYBIND XX)` 选项

### 脚本编译

项目根目录下提供了编译脚本文件，运行后将在`libs/`目录下生成目标

### 手动编译

1. 克隆代码仓库：

   ```bash
   git clone https://github.com/Micahcce/VisionFlux.git
   cd VisionFlux/native_cpp
   ```

2. 创建构建目录：

   ```
   mkdir build && cd build
   ```

3. 配置项目：`使用GNU编译，生成文件不包含 Qt 动态库`

   ```
   (Windows) cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
   (Linux) cmake ..
   ```

4. 编译项目：

   ```
   cmake --build .
   ```

5. 运行播放器：

   ```
   cd ../../libs
   ./VisionFlux
   ```

### Python 集成（可选）

如果你希望在 Python 中使用该播放器模块，可以通过以下步骤进行构建：

1. 启用 Python 绑定选项并重新编译项目：

   ```
   cmake -DENABLE_PYBIND=ON ..
   cmake --build .
   ```

2. 将在 Debug 或当前目录下生成二进制( .pyd或.so )文件

3. 在 Python 中导入模块：

   ```
   import visionflux
   ```

* **模块依赖于libs下的库文件，需要导入目录或者放在同一目录。**

* **python执行路径如果与模块不在同一目录，则也需要进行导入。**

*运行示例代码：*`python3 main.py`

## 未来计划

-  实现基于 AI 的媒体处理功能（如：自动生成字幕、字幕翻译、内容识别、基于视频内容的问答）。
-  支持更多的媒体格式和流媒体协议。

## 贡献

欢迎贡献！请 fork 本项目，提交您的修改，并通过 Pull Request 提交。

## 许可

VisionFlux 使用 MIT 许可证开源。

## 联系方式

如有任何问题或建议，欢迎在 GitHub 提交 issue 或联系项目维护者。
