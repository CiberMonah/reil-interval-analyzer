cmake_minimum_required(VERSION 3.16)

project(reil_interval_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(reil-core
    src/Parser.cpp
    src/Interval.cpp
    src/CFG.cpp
    src/Analyser.cpp
)

target_include_directories(reil-core PUBLIC src)

add_executable(reil-analyzer src/main.cpp)
target_link_libraries(reil-analyzer PRIVATE reil-core)

enable_testing()
add_subdirectory(tests)
