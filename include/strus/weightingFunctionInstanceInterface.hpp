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
/// \brief Interface for a parameterized weighting function instance
/// \file weightingFunctionInstanceInterface.hpp
#ifndef _STRUS_WEIGHTING_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_WEIGHTING_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#include "strus/globalStatistics.hpp"
#include <string>

namespace strus
{
/// \brief Forward declaration
class WeightingFunctionContextInterface;
/// \brief Forward declaration
class ArithmeticVariant;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class PostingIteratorInterface;

/// \brief Interface for a parameterized weighting function instance
class WeightingFunctionInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~WeightingFunctionInstanceInterface(){}

	/// \brief Add a named string value parameter
	/// \param[in] name parameter name
	/// \param[in] value parameter value
	virtual void addStringParameter( const std::string& name, const std::string& value)=0;

	/// \brief Add a named arithmetic value parameter
	/// \param[in] name parameter name
	/// \param[in] value parameter value
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value)=0;

	/// \brief Create an execution context for this weighting function instance
	/// \param[in] storage_ storage reference for retrieving some statistics (like the document collection frequency)
	/// \param[in] metadata meta data interface
	/// \param[in] stats global statistics
	/// \return the execution context to fetch the calculated document weights from (ownership to caller)
	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const=0;

	/// \brief Get a comma ',' separated list of the function parameters as assignments (e.g. name=value)
	/// \return the parameter list as string
	virtual std::string tostring() const=0;
};

}//namespace
#endif

