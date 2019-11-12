cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

add_test( QueryEval_bm25 ${CMAKE_CURRENT_BINARY_DIR}/src/testQueryEval 500 20 bm25 )
