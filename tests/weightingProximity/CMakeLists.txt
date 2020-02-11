cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

# Test of insert/retrieval of large random structures: testWeightingProximity  <method> <nofdocs> <nofterms> <nofnodes> <commitsize> <maxnofranks> <nofqry>
add_test( WeightingBM25_proximity_ff            ${CMAKE_CURRENT_BINARY_DIR}/src/testWeightingProximity  "bm25pff"  50  50  50  10  100  200 )
add_test( WeightingBM25_pure                       ${CMAKE_CURRENT_BINARY_DIR}/src/testWeightingProximity  "bm25"     50  50  50  10   100  200 )

