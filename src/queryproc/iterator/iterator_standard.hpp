/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Library providing some standard posting iterator join functions
#ifndef _STRUS_ITERATOR_STANDARD_LIB_HPP_INCLUDED
#define _STRUS_ITERATOR_STANDARD_LIB_HPP_INCLUDED

namespace strus {
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Forward declaration
class PostingJoinOperatorInterface;

/// \brief Create a join function returning the intersection of its argument sets
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinIntersect( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the union of its argument sets
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinUnion( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the predecessor set of the argument set (one argument only)
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingPred( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the predecessor set of the argument set (one argument only)
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingSucc( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets at different positions.
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinWithin( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets at different positions. Additionally there exists a restricting structure element (like for example punctuation).
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinStructWithin( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets.
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinInRange( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets. Additionally there exists a restricting structure element (like for example punctuation).
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinStructInRange( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets in the strict order they apear as arguments
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinSequence( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets in the strict order they apear as arguments
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinSequenceImm( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets in the strict order they apear as arguments. Additionally there exists a restricting structure element (like for example punctuation)
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinStructSequence( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets in the order they apear as arguments
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinChain( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets in the order they apear as arguments. Additionally there exists a restricting structure element (like for example punctuation)
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinStructChain( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning one element (with position = 1) for each document containing all elements of all argument sets somewhere (position independent logical AND for use as pure document selection feature)
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinContains( ErrorBufferInterface* errorhnd);

/// \brief Create a join function returning the all elements of the first argument set that does not overlap with one of the second
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinDifference( ErrorBufferInterface* errorhnd);

/// \brief Create a join function that selects the position from the first argument iterator that are between the second argument iterator and the third argument iterator
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinBetween( ErrorBufferInterface* errorhnd);

}//namespace
#endif

