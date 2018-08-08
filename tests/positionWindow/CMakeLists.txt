cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

add_test( PositionWindow ${CMAKE_CURRENT_BINARY_DIR}/src/testPositionWindow )

