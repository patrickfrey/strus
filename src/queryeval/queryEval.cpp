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
#include "queryEval.hpp"
#include "query.hpp"
#include "lexems.hpp"
#include "keyMap.hpp"
#include "termDef.hpp"
#include "summarizerDef.hpp"
#include "weightingFunctionDef.hpp"
#include "mapFunctionParameters.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/storageInterface.hpp"
#include "strus/constants.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include "queryEvalParser.hpp"
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <boost/scoped_ptr.hpp>
#include <boost/algorithm/string.hpp>

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

void QueryEval::loadProgram( const std::string& source)
{
	QueryEvalParser parser( m_processor);
	parser.loadProgram( *this, source);
}

void QueryEval::defineTerm( const TermDef& termDef)
{
	m_terms.push_back( termDef);
}

void QueryEval::defineWeightingFunction( const WeightingFunctionDef& funcDef)
{
	if (m_weightingFunction.function)
	{
		throw std::runtime_error( "more than one weighting function defined");
	}
	m_weightingFunction = funcDef;
}

void QueryEval::defineSummarizerDef( const SummarizerDef& sumDef)
{
	m_summarizers.push_back( sumDef);
}

void QueryEval::print( std::ostream& out) const
{
	std::vector<TermDef>::const_iterator ti = m_terms.begin(), te = m_terms.end();
	for (; ti != te; ++ti)
	{
		out << "TERM " << ti->set << ": " << ti->type << " '" << ti->value << "';" << std::endl;
	}
	if (m_weightingFunction.function)
	{
		out << "EVAL ";
		out << " " << m_weightingFunction.functionName;
		if (m_weightingFunction.parameters.size())
		{
			out << "<";
			std::size_t ai = 0, ae = m_weightingFunction.parameters.size();
			for(; ai != ae; ++ai)
			{
				if (ai) out << ", ";
				out << m_weightingFunction.function->parameterNames()[ai]
					<< "=" << m_weightingFunction.parameters[ai];
			}
			out << ">";
		}
		if (m_weightingFunction.selectorSets.size())
		{
			out << "[";
			std::size_t si = 0, se = m_weightingFunction.selectorSets.size();
			for(; si != se; ++si)
			{
				if (si) out << ", ";
				out << m_weightingFunction.selectorSets[si];
			}
			out << "]";
		}
		if (m_weightingFunction.weightingSets.size())
		{
			out << "(";
			std::size_t wi = 0, we = m_weightingFunction.weightingSets.size();
			for(; wi != we; ++wi)
			{
				if (wi) out << ", ";
				out << m_weightingFunction.weightingSets[wi];
			}
			out << ")";
		}
		out << ";" << std::endl;
	}
	std::vector<SummarizerDef>::const_iterator
		si = m_summarizers.begin(), se = m_summarizers.end();
	for (; si != se; ++si)
	{
		out << "SUMMARIZE ";
		out << si->resultAttribute << " = " << si->functionName;
		if (si->parameters.size())
		{
			out << "<";
			std::size_t ai = 0, ae = si->parameters.size();
			for(; ai != ae; ++ai)
			{
				if (ai) out << ", ";
				out << si->function->parameterNames()[ai] << "=" << si->parameters[ai];
			}
			out << ">";
		}
		if (!si->contentType.empty())
		{
			out << "[" << si->contentType << "]";
		}
		if (si->featureSet.size() || si->structSet.size())
		{
			out << "(";
			if (!si->structSet.empty())
			{
				out << si->structSet << ":";
			}
			std::size_t fi = 0, fe = si->featureSet.size();
			for(; fi != fe; ++fi)
			{
				if (fi) out << ", ";
				out << si->featureSet[fi];
			}
			out << ")";
		}
		out << ";" << std::endl;
	}
}


QueryInterface* QueryEval::createQuery( const StorageInterface* storage) const
{
	return new Query( this, storage, m_processor);
}

