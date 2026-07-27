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
    Search/openingbook.cpp
    Search/transpositiontable.cpp
    Search/moveordering.cpp
)

target_compile_options(ChessEngine PRIVATE -O3)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Resources 
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(WIN32)
    if(MINGW)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -static-libgcc -static-libstdc++")
    endif()
endif()