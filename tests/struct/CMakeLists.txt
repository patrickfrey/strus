cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

# Test of insert/retrieval of large random structures: testLargeStructures  <iterations>  <nofdocs> <nofstructs> <nofmembers> <commitsize> <nofqry>
add_test( LargeStructuresInsert         ${CMAKE_CURRENT_BINARY_DIR}/src/testLargeStructures  1 20 50 1000 4 5000 )
add_test( LargeStructuresReinsert     ${CMAKE_CURRENT_BINARY_DIR}/src/testLargeStructures  4 20 10   500 5 5000 )

