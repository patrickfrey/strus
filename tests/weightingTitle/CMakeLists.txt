cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

# Test of insert/retrieval of large random structures: testWeightingTitle  <nofdocs> <treesize> <commitsize> <nofqry>
add_test( WeightingTitle         ${CMAKE_CURRENT_BINARY_DIR}/src/WeightingTitle  500  500  4  50 )

