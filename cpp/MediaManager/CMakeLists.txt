cmake_minimum_required(VERSION 3.5)

project(MediaManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含当前路径
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# QT文件工具
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 查找QT库
find_package(Qt5 COMPONENTS Widgets Multimedia REQUIRED)

# 源文件
set(PROJECT_SOURCES
    main.cpp
    MediaManager.h
    MediaManager.cpp
    FrameQueue.h
    MainWindow.h
    MainWindow.cpp
    SdlPlayer.h
    SdlPlayer.cpp
    ButtomBar.h
    ButtomBar.cpp
    SideBar.h
    SideBar.cpp
)

# 创建可执行文件，并指定输出目录为项目的根目录
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)


# 设置库的包含目录（包含头文件）
include_directories(${PROJECT_SOURCE_DIR}/FFmpeg/include
    ${PROJECT_SOURCE_DIR}/SDL/include)

# 设置库目录（包含.lib文件）
link_directories(${PROJECT_SOURCE_DIR}/FFmpeg/lib
    ${PROJECT_SOURCE_DIR}/SDL/lib)

# 添加库到目标（例如 avformat, avcodec 等）
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/FFmpeg/lib/avformat.lib
    ${PROJECT_SOURCE_DIR}/FFmpeg/lib/avcodec.lib
    ${PROJECT_SOURCE_DIR}/FFmpeg/lib/avutil.lib
    ${PROJECT_SOURCE_DIR}/FFmpeg/lib/avdevice.lib
    ${PROJECT_SOURCE_DIR}/FFmpeg/lib/avfilter.lib
    ${PROJECT_SOURCE_DIR}/FFmpeg/lib/swscale.lib
    ${PROJECT_SOURCE_DIR}/FFmpeg/lib/swresample.lib
    ${PROJECT_SOURCE_DIR}/FFmpeg/lib/postproc.lib

    ${PROJECT_SOURCE_DIR}/SDL/lib/SDL2.lib
    ${PROJECT_SOURCE_DIR}/SDL/lib/SDL2main.lib
    ${PROJECT_SOURCE_DIR}/SDL/lib/SDL2test.lib

    Qt5::Core
    Qt5::Widgets
    Qt5::Multimedia
)
