/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Library providing some standard posting iterator join functions
#ifndef _STRUS_ITERATOR_STANDARD_LIB_HPP_INCLUDED
#define _STRUS_ITERATOR_STANDARD_LIB_HPP_INCLUDED

namespace strus {

/// \brief Forward declaration
class PostingJoinOperatorInterface;

/// \brief Create a join function returning the intersection of its argument sets
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinIntersect();

/// \brief Create a join function returning the union of its argument sets
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinUnion();

/// \brief Create a join function returning the predecessor set of the argument set (one argument only)
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingPred();

/// \brief Create a join function returning the predecessor set of the argument set (one argument only)
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingSucc();

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets.
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinWithin();

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets. Additionally there exists a restricting structure element (like for example punctuation).
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinStructWithin();

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets in the order they apear as arguments
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinSequence();

/// \brief Create a join function returning the first or last element of each passage in a range containing all elements of the argument sets in the order they apear as arguments. Additionally there exists a restricting structure element (like for example punctuation)
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinStructSequence();

/// \brief Create a join function returning one element (with position = 1) for each document containing all elements of all argument sets somewhere (position independent logical AND for use as pure document selection feature)
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinContains();

/// \brief Create a join function returning the all elements of the first argument set that does not overlap with one of the second
/// \return the iterator reference (to dispose with delete)
PostingJoinOperatorInterface* createPostingJoinDifference();

}//namespace
#endif

