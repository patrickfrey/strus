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
#ifndef _STRUS_SUMMARIZER_FUNCTION_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_FUNCTION_INTERFACE_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/arithmeticVariant.hpp"
#include <vector>

namespace strus
{
/// \brief Forward declaration
class StorageInterface;
/// \brief Forward declaration
class PostingIteratorInterface;
/// \brief Forward declaration
class ForwardIteratorInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class SummarizerClosureInterface;


/// \brief Interface for summarization (additional info about the matches in the result ranklist of a retrieval query)
class SummarizerFunctionInterface
{
public:
	virtual ~SummarizerFunctionInterface(){}

	/// \brief Get the name of the function
	/// \return the name of the function
	virtual const char* name() const=0;

	/// \brief Get the parameter names of the function in the order they should be passed
	/// \return the NULL terminated list of parameter names
	virtual const char** parameterNames() const=0;

	/// \brief Create a closure for this summarization function on a posting iterator reference and a meta data reader reference with an optional posting iterator describing a structure context
	/// \param[in] storage_ storage interface for getting info (like for example document attributes)
	/// \param[in] elementname_ name of element to select for summarization
	/// \param[in] structitr_ posting iterator describing the structural context of summarization (like for example sentence delimiters)
	/// \param[in] nofitrs_ number of elements in 'itrs_'
	/// \param[in] itrs_ reference to posting iterators that are subject of summarization
	/// \param[in] metadata_ meta data interface
	/// \param[in] parameters_ additional parameters for summarization
	/// \return the closure with some global statistics calculated only once
	virtual SummarizerClosureInterface* createClosure(
			const StorageInterface* storage_,
			const char* elementname_,
			PostingIteratorInterface* structitr_,
			std::size_t nofitrs_,
			PostingIteratorInterface** itrs_,
			MetaDataReaderInterface* metadata_,
			const std::vector<ArithmeticVariant>& parameters_) const=0;
};

}//namespace
#endif


