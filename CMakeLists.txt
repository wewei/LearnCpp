cmake_minimum_required(VERSION 3.12)
project(LearnCpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_executable(LearnCpp
    src/main.cc
    src/rvo.cc
    src/coroutine.cc
    src/tasks.cc
    src/tasks2.cc
)