cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    enable_language(ASM_MASM)
elseif(UNIX)
    enable_language(ASM)
endif()

file(GLOB lzma_resouce
    CPP/7zip/Compress/*.cpp
    CPP/7zip/Crypto/*.cpp
    C/*.c
    CPP/Common/*.cpp
    CPP/7zip/Common/*.cpp
    CPP/Windows/*.cpp
    CPP/7zip/Archive/*.cpp
    CPP/7zip/Archive/Common/*.cpp
    CPP/7zip/Archive/7z/*.cpp
    CPP/7zip/Archive/Archive2.def
    C/Util/LzmaLib/LzmaLib.def)

add_library(7zip ${lzma_resouce})
