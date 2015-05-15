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
#ifndef _STRUS_SUMMARIZER_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_SUMMARIZER_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief Forward declaration
class ArithmeticVariant;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class SummarizerExecutionContextInterface;


/// \brief Interface for a parameterized instance of summarization
class SummarizerFunctionInstanceInterface
{
public:
	/// \brief Destructor
	virtual ~SummarizerFunctionInstanceInterface(){}

	/// \brief Add a named string value parameter
	/// \param[in] name parameter name
	/// \param[in] value parameter value
	virtual void addStringParameter( const std::string& name, const std::string& value)=0;

	/// \brief Add a named arithmetic value parameter
	/// \param[in] name parameter name
	/// \param[in] value parameter value
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value)=0;

	/// \brief Check if a parameter name defines a feature parameter and not a numeric or string parameter
	/// \param[in] name parameter name
	/// \return true, if 'name' is the name of a feature parameter, false if not
	virtual bool isFeatureParameter( const std::string& name) const=0;

	/// \brief Create an execution context for this summarization function instance
	/// \param[in] storage_ storage interface for getting information for summarization (like for example document attributes)
	/// \param[in] metadata_ metadata interface for inspecting document meta data (like for example the document insertion date)
	/// \return the execution context, the summarization function instance with its execution context (ownership to caller)
	virtual SummarizerExecutionContextInterface* createExecutionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata_) const=0;

	/// \brief Get a comma ',' separated list of the function parameters as assignments (e.g. name=value)
	/// \return the parameter list as string
	virtual std::string tostring() const=0;
};

}//namespace
#endif


