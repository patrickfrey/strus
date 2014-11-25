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
#ifndef _STRUS_QUERY_PROCESSOR_HPP_INCLUDED
#define _STRUS_QUERY_PROCESSOR_HPP_INCLUDED
#include "strus/queryProcessorInterface.hpp"
#include "storageReference.hpp"
#include <vector>
#include <string>
#include <boost/shared_ptr.hpp>

namespace strus
{
/// \brief Forward declaration
class StorageInterface;


/// \brief Provides the objects needed for query processing
class QueryProcessor
	:public QueryProcessorInterface
{
public:
	/// \brief Constructor
	explicit QueryProcessor( StorageInterface* storage_);

	/// \brief Destructor
	virtual ~QueryProcessor(){}

	virtual PostingIteratorInterface*
		createTermPostingIterator(
			const std::string& type,
			const std::string& value) const;

	virtual PostingIteratorInterface*
		createJoinPostingIterator(
			const std::string& name,
			int range,
			std::size_t nofargs,
			const PostingIteratorInterface** args) const;

	virtual WeightingFunctionInterface*
		createWeightingFunction(
			const std::string& name,
			const std::vector<float>& parameter,
			const MetaDataReaderInterface* metadata) const;

	virtual SummarizerInterface*
		createSummarizer(
			const std::string& name,
			const std::string& type,
			const std::vector<float>& parameter,
			const PostingIteratorInterface* structitr,
			std::size_t nofitrs,
			const PostingIteratorInterface** itrs,
			const AttributeReaderInterface* attreader) const;

private:
	StorageInterface* m_storage;
};

}//namespace
#endif

