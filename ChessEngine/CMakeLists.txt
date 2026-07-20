cmake_minimum_required(VERSION 3.10)
project(ChessEngine)

set(CMAKE_CXX_STANDARD 20)

add_executable(ChessEngine
    main.cpp
    BoardRepresentation/board.cpp
    BoardRepresentation/movegenerator.cpp
    Search/search.cpp
    Evaluation/evaluation.cpp
    BoardRepresentation/zobrist.cpp
    engine.cpp
)

target_compile_options(ChessEngine PRIVATE -O3)