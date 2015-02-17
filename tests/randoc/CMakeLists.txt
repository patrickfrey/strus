cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

add_test( RandomCollection src/testRandomCollection path=data/test 200 200 100 300 )
