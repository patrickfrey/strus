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
/// \brief Forward declaration
class ForwardIndexViewerInterface;
/// \brief Forward declaration
class IteratorInterface;


class SummarizerMatchPhrase
	:public SummarizerInterface
{
public:
	SummarizerMatchPhrase(
		StorageInterface* storage_,
		const std::string& termtype_,
		int maxlen_);

	virtual ~SummarizerMatchPhrase();

	/// \brief Get the summarization based on term occurrencies
	/// \param[in,out] res where to append the summarization result
	/// \param[in] docno document to get the summary element from or 0, if the summary should be global
	/// \param[in] pos position tp get the summary element from element or 0, if the summary should be for the whole document
	/// \param[in] itr iterator for the term occurrencies where to get the summary from
	/// \param[in] markitr iterator for context markers related to the summary
	/// \return the summarization elements
	virtual bool
		getSummary(
			std::vector<SummaryElement>& res,
			const Index& docno,
			const Index& pos,
			IteratorInterface& itr,
			IteratorInterface& markitr);

private:
	StorageInterface* m_storage;
	ForwardIndexViewerInterface* m_forwardindex;
	std::string m_termtype;
	int m_maxlen;
};

}//namespace
#endif


