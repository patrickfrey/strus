cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

# Test of insert/retrieval of large random structures: testWeightingProximity  <nofdocs> <nofterms> <nofnodes> <commitsize> <nofqry>
add_test( WeightingProximity         ${CMAKE_CURRENT_BINARY_DIR}/src/testWeightingProximity  10  50  50  4  100 )

