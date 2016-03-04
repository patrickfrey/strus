cmake_minimum_required(VERSION 2.6 FATAL_ERROR)

# --------------------------------------
# SOURCES AND INCLUDES
# --------------------------------------
set(source_files
	iterator_standard.cpp
	postingIteratorIntersect.cpp
	postingIteratorUnion.cpp
	postingIteratorDifference.cpp
	postingIteratorSucc.cpp
	postingIteratorPred.cpp
	postingIteratorStructWithin.cpp
	postingIteratorStructSequence.cpp
	postingIteratorHelpers.cpp
	postingIteratorContains.cpp
	docnoMatchPrioQueue.cpp
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/src/queryproc/iterator"
)

# -------------------------------------
# LIBRARY
# -------------------------------------
add_library( iterator_standard STATIC ${source_files})
target_link_libraries( iterator_standard strus_utils )



