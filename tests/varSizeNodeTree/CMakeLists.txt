cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

add_test( VariableSizeNodeTree src/testVarSizeNodeTree 10000 100000)

