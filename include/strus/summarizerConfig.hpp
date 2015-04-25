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
#include <vector>
#include <utility>
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

	/// \brief Defines a string value parameter to pass to the summarizer
	/// \param[in] name_ name of the parameter
	/// \param[in] value_ value of the parameter
	void defineStringParameter( const std::string& name_, const std::string& value_);

	/// \brief References a feature parameter (set of postings) to pass as PostingIteratorInterface to the summarizer
	/// \param[in] name_ name of the feature parameter for the summarizer
	/// \param[in] set_ feature set name
	void addFeatureParameter( const std::string& name_, const std::string& set_);

	/// \brief Get all numeric parameter definitions of the summarizer
	/// \return the numeric parameter list
	const std::map<std::string,ArithmeticVariant>& numericParameters() const		{return m_numericParameters;}
	/// \brief Get all textual parameter definitions of the summarizer
	/// \return the textual parameter list
	const std::map<std::string,std::string>& stringParameters() const			{return m_stringParameters;}
	/// \brief Get all feature parameter definitions of the summarizer
	/// \return the feature parameter list
	const std::vector<std::pair<std::string,std::string> >& featureParameters() const	{return m_featureParameters;}

private:
	std::map<std::string,ArithmeticVariant> m_numericParameters;		///< the numeric parameter definition list
	std::map<std::string,std::string> m_stringParameters;			///< the textual parameter definition list
	std::vector<std::pair<std::string,std::string> > m_featureParameters;	///< the feature parameter definition list
};

}//namespace
#endif

