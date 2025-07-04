cmake_minimum_required(VERSION 3.24)

project(7z-pwd-finder)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加LZMA SDK包含路径
include_directories(
    lzma_sdk/C
)

# 定义LZMA SDK核心源文件
file(GLOB LZMA_SOURCES
    "lzma_sdk/C/*.c"
)

add_executable(7z-pwd-finder src/main.cpp ${LZMA_SOURCES})
target_include_directories(7z-pwd-finder PRIVATE
    lzma_sdk/C)

if(WIN32)
    target_compile_definitions(7z-pwd-finder PRIVATE 
        _WIN32
        _NO_CRT_STDIO_INLINE
    )
    target_link_libraries(7z-pwd-finder ole32.lib)
else()
    target_compile_definitions(7z-pwd-finder PRIVATE 
        _FILE_OFFSET_BITS=64
    )
endif()
