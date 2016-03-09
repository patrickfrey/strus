/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
/// \brief Element of summary (result of summarization)
/// \file summaryElement.hpp
#ifndef _STRUS_SUMMARY_ELEMENT_HPP_INCLUDED
#define _STRUS_SUMMARY_ELEMENT_HPP_INCLUDED
#include <string>

namespace strus
{

/// \brief One result element of summarization
class SummaryElement
{
public:
	/// \brief Constructor
	SummaryElement( const std::string& name_, const std::string& value_, double weight_=1.0, int index_=-1)
		:m_name(name_),m_value(value_),m_weight(weight_),m_index(index_){}
	/// \brief Copy constructor
	SummaryElement( const SummaryElement& o)
		:m_name(o.m_name),m_value(o.m_value),m_weight(o.m_weight),m_index(o.m_index){}

	/// \brief Get the name of the element
	const std::string& name() const		{return m_name;}
	/// \brief Get the value of the element
	const std::string& value() const	{return m_value;}
	/// \brief Get the weight of the element if defined
	double weight() const			{return m_weight;}
	/// \brief Get the index of the element if defined
	int index() const			{return m_index;}

private:
	std::string m_name;	///< name of the element
	std::string m_value;	///< content value of the element
	double m_weight;	///< weight of the element if defined
	int m_index;		///< index for multiple results (results with same index belong to the same group)
};

} //namespace
#endif

