/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Description of a weighting or summarizer function 
/// \file functionDescription.hpp
#ifndef _STRUS_FUNCTION_DESCRIPTION_HPP_INCLUDED
#define _STRUS_FUNCTION_DESCRIPTION_HPP_INCLUDED
#include "strus/numericVariant.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Structure that describes a function (weighting or summarizer function) for introspection
class FunctionDescription
{
public:
	/// \brief Structure that describes a parameter
	struct Parameter
	{
		enum Type
		{
			Feature,		///< parameter specifies a feature value passed as posting iterator to the function context
			Attribute,		///< parameter specifies a document attribute identifier
			Metadata,		///< parameter specifies a document metadata identifier
			Numeric,		///< parameter specifies a numeric constant (NumericVariant type)
			String			///< parameter specifies a string constant or enumeration item as string
		};

		/// \brief Constructor
		Parameter( const Type& type_, const std::string& name_, const std::string& text_, const std::string& domain_)
			:m_type(type_),m_name(name_),m_text(text_),m_domain(domain_){}
		/// \brief Copy constructor
		Parameter( const Parameter& o)
			:m_type(o.m_type),m_name(o.m_name),m_text(o.m_text),m_domain(o.m_domain){}

		/// \brief Get the type of the parameter
		Type type() const			{return m_type;}
		
		/// \brief Get the name of the parameter
		const std::string& name() const		{return m_name;}

		/// \brief Get the description text of the parameter
		const std::string& text() const		{return m_text;}

		/// \brief Get the description of the parameter domain
		/// \return List of values as string.
		///	Alternative values for the same element separated by '|'
		///	Different values allowed separated by ','
		///	Start and end of a value range separated by ':'
		///	Operator precedence '|' > ',' > ':' (as expected)
		///	Example 1: boolean with different alternatives: "true|t|y|1,false|f|n|0"
		///	Example 2: positive numbers smaller than 100: "1:99"
		///	Example 3: english weekdays: "Monday,Tuesday,Wednesday,Thursday,Friday,Saturday,Sunday"
		///	Example 4: non negative number: "0:"
		///	Example 5: anything: "" (empty string)
		/// \remark Its considered to be good practice to be case insensitive for enumeration values
		const std::string& domain() const	{return m_domain;}

	private:
		Type m_type;			///< type of parameter
		std::string m_name;		///< name of parameter
		std::string m_text;		///< textual description of parameter
		std::string m_domain;		///< description of the parameter domain
	};

public:
	/// \brief Add a parameter description
	FunctionDescription& operator()( Parameter::Type type_, const std::string& name_, const std::string& text_, const std::string& domain_=std::string())
	{
		m_param.push_back( Parameter( type_, name_, text_, domain_));
		return *this;
	}

	/// \brief Default constructor
	FunctionDescription(){}

	/// \brief Constructor
	explicit FunctionDescription( const std::string& text_)
		:m_text(text_){}

	/// \brief Copy constructor
	FunctionDescription( const FunctionDescription& o)
		:m_text(o.m_text),m_param(o.m_param){}

	/// \brief Get the description text
	const std::string& text() const				{return m_text;}

	/// \brief Get the description parameter list
	const std::vector<Parameter>& parameter() const		{return m_param;}

private:
	std::string m_text;		///< description text
	std::vector<Parameter> m_param;	///< list of parameter descriptions
};

} //namespace
#endif


