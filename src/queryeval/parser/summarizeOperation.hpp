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
#ifndef _STRUS_QUERY_PARSER_SUMMARIZE_OPERATION_HPP_INCLUDED
#define _STRUS_QUERY_PARSER_SUMMARIZE_OPERATION_HPP_INCLUDED
#include "parser/stringIndexMap.hpp"
#include <string>
#include <vector>

namespace strus {
namespace parser {

#error DEPRECATED

/// \brief Defines a summarizer for adding attributes to matches
class SummarizeOperation
{
public:
	SummarizeOperation()
		:m_structSet(0){}

	SummarizeOperation(
		const std::string& resultAttribute_,
		const std::string& summarizerName_,
		const std::string& type_,
		const std::vector<float>& parameter_,
		const std::vector<int>& featureSet_,
		int structSet_);

	SummarizeOperation( const SummarizeOperation& o);

	void parse( char const*& src, StringIndexMap& setnamemap);
	void print( std::ostream& out, const StringIndexMap& setnamemap) const;

public:
	std::string resultAttribute() const		{return m_resultAttribute;}
	std::string summarizerName() const		{return m_summarizerName;}
	std::string type() const			{return m_type;}
	const std::vector<float>& parameter() const	{return m_parameter;}
	const std::vector<int>& featureset() const	{return m_featureSet;}
	int structset() const				{return m_structSet;}

private:
	std::string m_resultAttribute;
	std::string m_summarizerName;
	std::string m_type;
	std::vector<float> m_parameter;
	std::vector<int> m_featureSet;
	int m_structSet;
};

}}//namespace
#endif

