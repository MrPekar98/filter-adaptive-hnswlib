cmake_minimum_required(VERSION 3.10)
project(acorn_example CXX)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ${CMAKE_SOURCE_DIR}/ACORN
    ${CMAKE_SOURCE_DIR}/ACORN/faiss
)

add_executable(index_acorn index_acorn.cpp)
target_link_libraries(index_acorn
    ${CMAKE_SOURCE_DIR}/ACORN/build/faiss/libfaiss.so
    pthread
    m
    gomp
)
