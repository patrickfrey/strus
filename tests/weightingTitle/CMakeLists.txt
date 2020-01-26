cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

# Test of insert/retrieval of large random structures: testWeightingTitle  <nofdocs> <nofterms> <nofnodes> <commitsize> <nofqry>
add_test( WeightingTitle         ${CMAKE_CURRENT_BINARY_DIR}/src/WeightingTitle  1  500  5  4  1 )

