cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

add_test( RandomCollection src/testRandomCollection path=storage 100 100 100 200 )
