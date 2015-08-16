cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

add_subdirectory(src)

add_test( PeerMessageProcessor src/testPeerMessageProcessor 100000 10000 1000 )
