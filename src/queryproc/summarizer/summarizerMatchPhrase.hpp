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
#include "private/iteratorReference.hpp"
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
	/// \param[in] storage_ storage to use
	/// \param[in] termtype_ type of the tokens to build the summary with
	/// \param[in] maxlen_ maximum lenght of a sentence on both sides of the matching feature until it is cut and terminated with "..."
	/// \param[in] summarylen_ maximum lenght of the whole summary
	/// \param[in] nofitrs_ number of argument iterators
	/// \param[in] itrs_ argument iterators
	/// \param[in] phrasestruct_ structure iterator to recognize end of phrases
	SummarizerMatchPhrase(
		StorageInterface* storage_,
		const std::string& termtype_,
		unsigned int maxlen_,
		unsigned int summarylen_,
		std::size_t nofitrs_,
		const IteratorInterface** itrs_,
		const IteratorInterface* phrasestruct_);

	virtual ~SummarizerMatchPhrase();

	/// \brief Get some summarization elements
	/// \param[in] docno document to get the summary element from
	/// \return the summarization elements
	virtual std::vector<std::string> getSummary( const Index& docno);

private:
	StorageInterface* m_storage;
	ForwardIndexViewerInterface* m_forwardindex;
	std::string m_termtype;
	unsigned int m_maxlen;
	unsigned int m_summarylen;
	std::vector<IteratorReference> m_itr;
	IteratorReference m_phrasestruct;
};

}//namespace
#endif

