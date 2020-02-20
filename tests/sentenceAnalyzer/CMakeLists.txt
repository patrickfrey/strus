cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

add_test( SentenceAnalyzer ${CMAKE_CURRENT_BINARY_DIR}/src/testSentenceAnalyzer 100 100 100 100 )

