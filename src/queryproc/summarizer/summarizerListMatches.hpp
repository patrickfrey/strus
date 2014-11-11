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
#ifndef _STRUS_SUMMARIZER_LIST_MATCHES_HPP_INCLUDED
#define _STRUS_SUMMARIZER_LIST_MATCHES_HPP_INCLUDED
#include "strus/summarizerInterface.hpp"
#include "private/postingIteratorReference.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

namespace strus
{

/// \brief Forward declaration
class StorageInterface;
/// \brief Forward declaration
class PostingIteratorInterface;


class SummarizerListMatches
	:public SummarizerInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] nofitrs_ number of argument iterators
	/// \param[in] itrs_ argument iterators
	SummarizerListMatches(
		StorageInterface* storage_,
		std::size_t nofitrs_,
		const PostingIteratorInterface** itrs_);

	virtual ~SummarizerListMatches();

	/// \brief Get some summarization elements
	/// \param[in] docno document to get the summary element from
	/// \return the summarization elements
	virtual std::vector<std::string> getSummary( const Index& docno);

private:
	StorageInterface* m_storage;
	std::vector<PostingIteratorReference> m_itr;
};

}//namespace
#endif


