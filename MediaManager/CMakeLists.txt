cmake_minimum_required(VERSION 3.15)

project(MediaManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 包含当前路径
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 定义基本源文件集
set(PROJECT_SOURCES
    MediaManager.h
    MediaManager.cpp
    PlayController.h
    PlayController.cpp
    SdlPlayer.h
    SdlPlayer.cpp

    common/FrameQueue.h
    common/SystemClock.h
    common/Logger.h
    common/Logger.cpp
    common/BmpAndWavAchieve.h
    common/BmpAndWavAchieve.cpp
    common/Utils.h
    common/Utils.cpp
)

# 是否开启Pybind11
option(ENABLE_PYBIND "enable pybind11 in order to create pyd files." OFF)
set(ENABLE_PYBIND OFF)

# 条件编译
if(ENABLE_PYBIND)
    message(STATUS "Pybind11 Enabled.")
    # PyBind11模块
    set(pybind11_DIR ${PROJECT_SOURCE_DIR}/pybind11/share/cmake/pybind11)
    find_package(pybind11 REQUIRED)

    pybind11_add_module(${PROJECT_NAME}
        ${PROJECT_SOURCES}
        common/BindClass.cpp
    )
    # 为代码添加ENABLE_PYBIND宏定义
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_PYBIND)

    # 设置 pyd 文件的输出路径
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build"
    )

else()
    message(STATUS "Pybind11 Disabled.")
    # QT文件工具
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)

    # 查找QT库
    find_package(Qt5 COMPONENTS Widgets REQUIRED)

    # 创建可执行文件
    add_executable(${PROJECT_NAME}
        ${PROJECT_SOURCES}
        main.cpp
        QtGUI/MainWindow.h
        QtGUI/MainWindow.cpp
        QtGUI/BottomBar.h
        QtGUI/BottomBar.cpp
        QtGUI/PlayList.h
        QtGUI/PlayList.cpp
        QtGUI/ProcessPanel.h
        QtGUI/ProcessPanel.cpp
    )

    # 指定输出目录为项目的根目录
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../libs"
    )
endif()


# 设置库的包含目录（包含头文件）
include_directories(
    ${PROJECT_SOURCE_DIR}/common
    ${PROJECT_SOURCE_DIR}/FFmpeg/include
    ${PROJECT_SOURCE_DIR}/SDL/include
    ${PROJECT_SOURCE_DIR}/SoundTouch/include
)

# 设置库目录（包含.lib文件）
link_directories(
    ${PROJECT_SOURCE_DIR}/FFmpeg/lib
    ${PROJECT_SOURCE_DIR}/SDL/lib
    ${PROJECT_SOURCE_DIR}/SoundTouch/lib
)

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

    ${PROJECT_SOURCE_DIR}/SoundTouch/lib/SoundTouchDLLD_x64.lib
)

# 补充
if(ENABLE_PYBIND)
    include_directories(${PROJECT_SOURCE_DIR}/pybind11/include)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        Qt5::Core
        Qt5::Widgets
    )
endif()

# Cuda加速
find_package(CUDA)
if(CUDA_FOUND)
    message(STATUS "CUDA successfully found!")
    include_directories(${CUDA_INCLUDE_DIRS})
    # 为代码添加ENABLE_PYBIND宏定义
    target_compile_definitions(${PROJECT_NAME} PRIVATE CUDA_ISAVAILABLE)
else()
    message(WARNING "CUDA not found, proceeding without CUDA support.")
endif()
