cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

add_subdirectory(src)

# Test of insert collection (with transaction commit size 20 documents) of 400 documents representing 
# a number (2..402) with its prime factors as features  
add_test( StorageInsertPrimeNumberDocs     ${CMAKE_CURRENT_BINARY_DIR}/src/testInsert  800  20 )

