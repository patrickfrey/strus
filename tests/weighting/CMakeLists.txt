cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

# Test of insert/retrieval/ranking of documents: testWeighting  <method> <nofdocs> <nofterms> <nofnodes> <commitsize> <maxnofranks> <nofqry>
add_test( WeightingBM25_pff                          ${CMAKE_CURRENT_BINARY_DIR}/src/testWeighting  "bm25pff"  50  50  50  10  100  200 )
add_test( WeightingBM25_pure                       ${CMAKE_CURRENT_BINARY_DIR}/src/testWeighting  "bm25"      50  50  50  10  100  200 )

