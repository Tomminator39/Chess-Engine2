cmake_minimum_required(VERSION 3.10)
project(ChessEngine)

set(CMAKE_CXX_STANDARD 20)

add_executable(ChessEngine
    main.cpp
)

target_compile_options(ChessEngine PRIVATE -O3)