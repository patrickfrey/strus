cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_subdirectory(src)

# SET_TESTS_PROPERTIES( LoggingMalloc PROPERTIES ENVIRONMENT "STRUS_MALLOC_LOGFILE=malloc.log" )
add_test( LoggingMalloc src/testLoggingMalloc 100000 100000000)

