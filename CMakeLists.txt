cmake_minimum_required(VERSION 3.16)

project(reil_interval_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(reil-analyzer
    src/main.cpp
    src/Parser.cpp
    src/Interval.cpp
    src/Analyser.cpp
)

target_include_directories(reil-analyzer PRIVATE src)

enable_testing()
add_subdirectory(tests)
