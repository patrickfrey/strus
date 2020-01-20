/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a parameterized weighting function instance
/// \file weightingFunctionInstanceInterface.hpp
#ifndef _STRUS_WEIGHTING_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#define _STRUS_WEIGHTING_FUNCTION_INSTANCE_INTERFACE_HPP_INCLUDED
#include "strus/globalStatistics.hpp"
#include "strus/structView.hpp"
#include <string>

namespace strus
{
/// \brief Forward declaration
class WeightingFunctionContextInterface;
/// \brief Forward declaration
class NumericVariant;
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

	/// \brief Set maximum number of weighted fields
	/// \param[in] N maximum number of weighted fields (default is 1)
	/// \note The default is 1 because most of the weighting methods just weight one document
	virtual void setMaxNofWeightedFields( int N)=0;

	/// \brief Add a named string value parameter
	/// \param[in] name parameter name
	/// \param[in] value parameter value
	virtual void addStringParameter( const std::string& name, const std::string& value)=0;

	/// \brief Add a named numeric value parameter
	/// \param[in] name parameter name
	/// \param[in] value parameter value
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value)=0;

	/// \brief Create an execution context for this weighting function instance
	/// \param[in] storage_ storage reference for retrieving some statistics (like the document collection frequency)
	/// \param[in] metadata_ meta data interface
	/// \param[in] stats global statistics for weighting
	/// \return the execution context to fetch the calculated document weights from (ownership to caller)
	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			const GlobalStatistics& stats) const=0;

	/// \brief Get the list of variables used by this function defined in the query with 'QueryInterface::setWeightingVariableValue( const std::string&, double)'
	/// \return the list of variables
	virtual std::vector<std::string> getVariables() const=0;

	/// \brief Get the name of the function
	/// \return the identifier
	virtual const char* name() const=0;

	/// \brief Return a structure with all definitions for introspection
	/// \return the structure with all definitions for introspection
	virtual StructView view() const=0;
};

}//namespace
#endif

