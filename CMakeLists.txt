cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(7z-pwd-finder)

add_subdirectory(7zip)

add_executable(7z-pwd-finder src/main.cpp)
target_link_libraries(7z-pwd-finder PRIVATE 7zip)
