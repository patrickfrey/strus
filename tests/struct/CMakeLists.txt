cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

# Test of insert/retrieval of large random structures: testLargeStructures  <nofdocs> <nofstructs> <commitsize> <nofqry>
add_test( LargeStructuresInsert         ${CMAKE_CURRENT_BINARY_DIR}/src/testLargeStructures  5  5000  4  50000 )

