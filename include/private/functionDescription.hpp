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
#include "strus/structView.hpp"
#include <string>
#include <vector>

namespace strus
{

/// \brief Structure that describes a function (weighting or summarizer function) for introspection
class FunctionDescription
	:public StructView
{
public:
	enum ParameterType
	{
		Feature,		///< parameter specifies a feature value passed as posting iterator to the function context
		Attribute,		///< parameter specifies a document attribute identifier
		Metadata,		///< parameter specifies a document metadata identifier
		Numeric,		///< parameter specifies a numeric constant (NumericVariant type)
		String			///< parameter specifies a string constant or enumeration item as string
	};
	static const char* parameterTypeName( ParameterType i)
	{
		static const char* ar[] = {"Feature","Attribute","Metadata","Numeric","String",0};
		return ar[i];
	}

public:
	/// \brief Add a parameter description
	FunctionDescription& operator()( ParameterType type_, const std::string& name_, const std::string& text_, const std::string& domain_=std::string())
	{
		StructView paramdef;
		paramdef("type", parameterTypeName(type_))("name", name_)("text", text_);
		if (!domain_.empty()) paramdef("domain", domain_);
		StructView* paramlist = this->get( "parameter");
		if (!paramlist)
		{
			((StructView)(*this))( "parameter", StructView());
			paramlist = this->get( "parameter");
		}
		(*paramlist)( paramdef);
		return *this;
	}

	/// \brief Default constructor
	FunctionDescription(){}

	/// \brief Constructor
	explicit FunctionDescription( const std::string& text_)
	{
		((StructView)(*this))( "text", text_);
	}

	/// \brief Derived constructor
	FunctionDescription( const StructView& o, const std::string& text_)
	{
		const StructView* old_textview = o.get( "text");
		((StructView)(*this))( "text", text_ + ": " + old_textview->tostring());
	}
};

} //namespace
#endif


