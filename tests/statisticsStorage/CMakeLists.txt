cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

add_test( StatisticsStorage ${CMAKE_CURRENT_BINARY_DIR}/src/testStatisticsStorage "path=storage" 10 1000 )
