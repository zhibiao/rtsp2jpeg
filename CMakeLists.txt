cmake_minimum_required(VERSION 3.10)

project(demo VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(
    "libs\\ffmpeg-4.2.2\\include"
)

link_directories(
    "libs\\ffmpeg-4.2.2\\lib"
)

link_libraries(
    avcodec.lib
    avdevice.lib
    avfilter.lib
    avformat.lib
    avutil.lib
    postproc.lib
    swresample.lib
    swscale.lib
)

add_executable(demo src/main.cpp)

set_target_properties(
    demo
    PROPERTIES 
    SUFFIX 
    ".exe"
)

install(
    TARGETS
    demo
    RUNTIME
    DESTINATION 
    ${PROJECT_SOURCE_DIR}
)