cmake_minimum_required(VERSION 3.21)
project(lab01 C)

include_directories(/usr/local/Cellar/mpich/3.4.3/include/)

set(CMAKE_C_STANDARD 99)

SET(CMAKE_C_COMPILER mpicc)
SET(CMAKE_CXX_COMPILER mpicxx)

add_executable(lab01 main.c board.c path_search.c error.c defines.h board.h path_search.h error.h)
