cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_subdirectory(src)

add_test( Armadillo src/testArmadillo )
add_test( VectorSimHash src/testLshSimHash )
add_test( LshVectorSpaceModel src/testLshVectorSpaceModel )
