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
#ifndef _STRUS_QUERY_PROCESSOR_INTERFACE_HPP_INCLUDED
#define _STRUS_QUERY_PROCESSOR_INTERFACE_HPP_INCLUDED
#include "strus/accumulatorInterface.hpp"
#include "strus/iteratorInterface.hpp"
#include "strus/weightedDocument.hpp"
#include <vector>
#include <string>

namespace strus
{

/// \brief Defines all object instances involved in query evaluation addressable by name
class QueryProcessorInterface
{
public:
	/// \brief Destructor
	virtual ~QueryProcessorInterface(){}

	/// \brief Create an iterator on the occurrencies of a term
	/// \param[in] type type name of the term
	/// \param[in] value value string of the term
	/// \return the created iterator reference object
	virtual IteratorInterface*
		createIterator(
			const std::string& type,
			const std::string& value)=0;

	/// \brief Create an iterator as join function on the arguments passed
	/// \param[in] name name of the join function to execute
	/// \param[in] range additional parameters for describing a range or offset for the join operator to execute
	/// \param[in] nofargs number of arguments to pass to the function
	/// \param[in] args arguments to pass to the function
	/// \return the created iterator reference object representing the result of the function
	virtual IteratorInterface*
		createIterator(
			const std::string& name,
			int range,
			std::size_t nofargs,
			const IteratorInterface** args)=0;

	struct WeightedAccumulator
	{
		double weight;
		const AccumulatorInterface* accu;

		explicit WeightedAccumulator( const AccumulatorInterface* accu_=0, double weight_=1.0)
			:weight(weight_),accu(accu_){}
		WeightedAccumulator( const WeightedAccumulator& o)
			:weight(o.weight),accu(o.accu){}
	};

	/// \brief Create an accumulator as join of the accumulators passed as argument
	/// \param[in] name name of the accumulator function to execute
	/// \param[in] scale constant factors used in the function
	/// \param[in] nofargs number of accumulator references with weights
	/// \param[in] args list of accumulator references with weights
	/// \return the created accumulator reference object representing the result of the function
	virtual AccumulatorInterface*
		createAccumulator(
			const std::string& name,
			const std::vector<double>& scale,
			std::size_t nofargs,
			const WeightedAccumulator* arg)=0;

	/// \brief Create an accumulator of the feature occurrence set passed as argument
	/// \param[in] name name of the accumulator operator to execute
	/// \param[in] nofargs number of argument iterators to pass to the function
	/// \param[in] args argument iterators to pass to the function
	virtual AccumulatorInterface*
		createOccurrenceAccumulator(
			const std::string& name,
			std::size_t nofargs,
			const IteratorInterface** args)=0;

	/// \brief Calculate a list of the best ranked documents
	/// \param[in] accu accumulator to fetch the weighted documents from
	/// \param[in] maxNofRanks maximum number of ranks to return
	virtual std::vector<WeightedDocument>
		getRankedDocumentList(
			AccumulatorInterface& accu,
			std::size_t maxNofRanks) const=0;
};

}//namespace
#endif

