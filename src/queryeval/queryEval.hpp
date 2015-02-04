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
#ifndef _STRUS_QUERY_PROGRAM_HPP_INCLUDED
#define _STRUS_QUERY_PROGRAM_HPP_INCLUDED
#include "strus/queryEvalInterface.hpp"
#include "strus/resultDocument.hpp"
#include "strus/arithmeticVariant.hpp"
#include "termDef.hpp"
#include "summarizerDef.hpp"
#include "weightingFunctionDef.hpp"
#include <string>
#include <vector>

namespace strus {

/// \brief Forward declaration
class SummarizerFunctionInterface;
/// \brief Forward declaration
class StorageInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class WeightingFunctionInterface;
/// \brief Forward declaration
class SummarizerFunctionInterface;

/// \brief Query evaluation program representation
class QueryEval
	:public QueryEvalInterface
{
public:
	explicit QueryEval( const QueryProcessorInterface* processor_)
		:m_processor(processor_){}

	QueryEval( const QueryEval& o)
		:m_processor(o.m_processor)
		,m_weightingFunction(o.m_weightingFunction)
		,m_summarizers(o.m_summarizers)
		,m_terms(o.m_terms)
	{}

	virtual void loadProgram( const std::string& source);
	virtual QueryInterface* createQuery( const StorageInterface* storage) const;

	const std::vector<TermDef>& terms() const			{return m_terms;}
	const std::vector<SummarizerDef>& summarizers() const		{return m_summarizers;}
	const WeightingFunctionDef& weightingFunction() const		{return m_weightingFunction;}

	void defineTerm( const TermDef& termDef);
	void defineWeightingFunction( const WeightingFunctionDef& funcDef);
	void defineSummarizerDef( const SummarizerDef& sumDef);

	virtual void print( std::ostream& out) const;

private:
	const QueryProcessorInterface* m_processor;

	WeightingFunctionDef m_weightingFunction;
	std::vector<SummarizerDef> m_summarizers;
	std::vector<TermDef> m_terms;
};

}//namespace
#endif

