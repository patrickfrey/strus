cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_subdirectory(src)

add_test( RandomCollection src/testRandomCollection path=storage 200 200 100 300 )
