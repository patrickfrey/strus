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
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/summarizerClosureInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Forward declaration
class StorageInterface;
/// \brief Forward declaration
class PostingIteratorInterface;


class SummarizerClosureListMatches
	:public SummarizerClosureInterface
{
public:
	/// \param[in] storage_ storage to use
	/// \param[in] nofitrs_ number of argument iterators
	/// \param[in] itrs_ argument iterators
	SummarizerClosureListMatches(
		const StorageInterface* storage_,
		const std::vector<PostingIteratorInterface*>& itrs_);

	virtual ~SummarizerClosureListMatches();

	/// \brief Get some summarization elements
	/// \param[in] docno document to get the summary element from
	/// \return the summarization elements
	virtual std::vector<std::string> getSummary( const Index& docno);

private:
	const StorageInterface* m_storage;
	std::vector<PostingIteratorInterface*> m_itrs;
};


class SummarizerFunctionListMatches
	:public SummarizerFunctionInterface
{
public:
	SummarizerFunctionListMatches(){}

	virtual ~SummarizerFunctionListMatches(){}

	virtual const char* name() const
	{
		return "matchpos";
	}

	virtual const char** parameterNames() const
	{
		static const char* ar[] = {0};
		return ar;
	}
	
	virtual SummarizerClosureInterface* createClosure(
			const StorageInterface* storage_,
			const char*,
			PostingIteratorInterface* structitr_,
			const std::vector<PostingIteratorInterface*>& itrs_,
			MetaDataReaderInterface*,
			const std::vector<ArithmeticVariant>&) const
	{
		if (structitr_) throw std::runtime_error( "no structural argument expected for summarizer 'matchpositions'");
		return new SummarizerClosureListMatches( storage_, itrs_);
	}
};


}//namespace
#endif


