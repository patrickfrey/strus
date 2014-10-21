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

namespace strus
{

/// \brief Provides the objects needed for query processing
class QueryProcessor
	:public QueryProcessorInterface
{
public:
	/// \brief Constructor
	explicit QueryProcessor( StorageInterface* storage_)
		:m_storage(storage_){}

	/// \brief Destructor
	virtual ~QueryProcessor(){}

	/// \brief Create an iterator on the occurrencies of a basic term in the collection
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \return the created iterator reference object
	virtual IteratorInterface*
		createTermIterator(
			const std::string& type,
			const std::string& value) const;

	/// \brief Create an iterator as join function on the arguments passed
	/// \param[in] name name of the join function to execute
	/// \param[in] range additional parameters for describing a range or offset for the join operator to execute
	/// \param[in] nofargs number of arguments to pass to the function
	/// \param[in] args arguments to pass to the function
	/// \return the created iterator reference object representing the result of the function
	virtual IteratorInterface*
		createJoinIterator(
			const std::string& name,
			int range,
			std::size_t nofargs,
			const IteratorInterface** args) const;

	/// \brief Create an accumulator for the summation of weighted term occurrencies
	/// \param[in] name name of the accumulator (defines the priorisation of ranking)
	/// \return the created accumulator object
	virtual AccumulatorInterface*
		createAccumulator(
			const std::string& name) const;

private:
	StorageInterface* m_storage;
};

}//namespace
#endif

