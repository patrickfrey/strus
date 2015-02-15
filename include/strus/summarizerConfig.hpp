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
#ifndef _STRUS_SUMMARIZER_CONFIG_HPP_INCLUDED
#define _STRUS_SUMMARIZER_CONFIG_HPP_INCLUDED
#include "strus/arithmeticVariant.hpp"
#include <string>
#include <map>

namespace strus {

/// \class SummarizerConfig
/// \brief Configuration of a query evaluation summarizer
class SummarizerConfig
{
public:
	/// \brief Destructor
	~SummarizerConfig(){}

	/// \brief Defines a numeric parameter to pass to the summarizer
	/// \param[in] name_ name of the parameter
	/// \param[in] value_ value of the parameter
	void defineNumericParameter( const std::string& name_, const ArithmeticVariant& value_);

	/// \brief Defines a textual parameter to pass to the summarizer
	/// \param[in] name_ name of the parameter
	/// \param[in] value_ value of the parameter
	void defineTextualParameter( const std::string& name_, const std::string& value_);

	/// \brief References a set of postings to pass as PostingIteratorInterface to the summarizer
	/// \param[in] class_ name of the feature set class for the summarizer. The class describes for what the feature is used.
	/// \param[in] set_ feature set name
	void defineFeatureParameter( const std::string& class_, const std::string& set_);

	const std::map<std::string,ArithmeticVariant>& numericParameters() const	{return m_numericParameters;}
	const std::map<std::string,std::string>& textualParameters() const		{return m_textualParameters;}
	const std::map<std::string,std::string>& featureParameters() const		{return m_featureParameters;}

private:
	std::map<std::string,ArithmeticVariant> m_numericParameters;
	std::map<std::string,std::string> m_textualParameters;
	std::map<std::string,std::string> m_featureParameters;
};

}//namespace
#endif

