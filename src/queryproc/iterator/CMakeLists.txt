cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

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
	postingIteratorSequenceImm.cpp
	postingIteratorHelpers.cpp
	postingIteratorContains.cpp
	docnoMatchPrioQueue.cpp
	docnoAllMatchItr.cpp
)

include_directories(
  ${Boost_INCLUDE_DIRS}
  "${STRUS_INCLUDE_DIRS}"
  "${MAIN_SOURCE_DIR}/queryproc/iterator"
  "${MAIN_SOURCE_DIR}/queryproc/utils"
  "${strusbase_INCLUDE_DIRS}"
)

link_directories(
  "${strusbase_LIBRARY_DIRS}"
  "${Intl_INCLUDE_DIRS}"
)

# -------------------------------------
# LIBRARY
# -------------------------------------
add_library( iterator_standard STATIC ${source_files})
target_link_libraries( iterator_standard strus_base )
set_property( TARGET iterator_standard PROPERTY POSITION_INDEPENDENT_CODE TRUE )



