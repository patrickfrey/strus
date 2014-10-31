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
#ifndef _STRUS_SUMMARIZER_MATCHPHRASE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_MATCHPHRASE_HPP_INCLUDED
#include "strus/summarizerInterface.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class StorageInterface;

class SummarizerMatchPhrase
	:public SummarizerInterface
{
public:
	SummarizerMatchPhrase( const StorageInterface* storage_, int maxlen)
		:m_storage(storage_)
		,m_maxlen(maxlen_){}

	virtual ~SummarizerMatchPhrase(){}

	/// \brief Get the summarization based on term occurrencies
	/// \param[in] docno document to get the summary from 
	/// \param[in] itr iterator for the term occurrencies to get the summary from 
	/// \param[in] markitr iterator for context markers related to the summary
	/// \return the summarization elements
	virtual std::vector<SummaryElement>
		getSummary(
			const Index& docno,
			IteratorInterface& itr,
			IteratorInterface& markitr);

private:
	const StorageInterface* m_storage;
	int m_maxlen;
};

}//namespace
#endif


