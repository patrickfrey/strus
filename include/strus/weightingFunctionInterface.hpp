/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a weighting function type
/// \file weightingFunctionInterface.hpp
#ifndef _STRUS_WEIGHTING_FUNCTION_INTERFACE_HPP_INCLUDED
#define _STRUS_WEIGHTING_FUNCTION_INTERFACE_HPP_INCLUDED
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class WeightingFunctionInstanceInterface;
/// \brief Forward declaration
class QueryProcessorInterface;

/// \brief Interface for a weighting function that can be used for ranking in the query evaluation
class WeightingFunctionInterface
{
public:
	/// \brief Destructor
	virtual ~WeightingFunctionInterface(){}

	/// \brief Create an instance of this function for parametrization
	/// \param[in] processor provider for query processing functions
	/// \return the created function instance (ownership to caller)
	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const=0;

	/// \brief Structure that describes the weighting function (for introspection)
	struct Description
	{
		/// \brief Structure that describes a parameter
		struct Param
		{
			enum Type
			{
				Feature,
				Attribute,
				Metadata,
				Numeric,
				String
			};
			/// \brief Get the name of the parameter type
			static const char* typeName( Type type_)
			{
				static const char* ar[] = {"Feature","Attribute","Metadata","Numeric","String"};
				return ar[ (unsigned int)type_];
		 	}
			/// \brief Get the name of this parameter type
			const char* typeName() const
			{
				return typeName( m_type);
			}

			/// \brief Constructor
			Param( Type type_, const std::string& name_, const std::string& text_, const std::string& domain_)
				:m_type(type_),m_name(name_),m_text(text_),m_domain(domain_){}
			/// \brief Copy constructor
			Param( const Param& o)
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
			/// \remark Its considered to be good practice to be case insensitive for enumeration values
			const std::string& domain() const	{return m_domain;}

		private:
			Type m_type;			///< type of parameter
			std::string m_name;		///< name of parameter
			std::string m_text;		///< description of parameter
			std::string m_domain;		///< description of the parameter domain
		};

		/// \brief Add a parameter description
		Description& operator()( Param::Type type_, const std::string& name_, const std::string& text_, const std::string& domain_=std::string())
		{
			m_param.push_back( Param( type_, name_, text_, domain_));
			return *this;
		}

		/// \brief Default constructor
		Description(){}

		/// \brief Constructor
		explicit Description( const std::string& text_)
			:m_text(text_){}

		/// \brief Copy constructor
		Description( const Description& o)
			:m_text(o.m_text),m_param(o.m_param){}

		/// \brief Get the description text
		const std::string& text() const			{return m_text;}
		/// \brief Get the description parameter list
		const std::vector<Param>& param() const		{return m_param;}

	private:
		std::string m_text;		///< description text
		std::vector<Param> m_param;	///< list of parameter descriptions
	};

	/// \brief Get a description of the function for user help and introspection
	/// \return the description structure
	virtual Description getDescription() const=0;
};

}//namespace
#endif

