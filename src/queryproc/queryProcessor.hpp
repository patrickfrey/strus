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
#ifndef _STRUS_QUERY_PROCESSOR_HPP_INCLUDED
#define _STRUS_QUERY_PROCESSOR_HPP_INCLUDED
#include "strus/queryProcessorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/summarizerFunctionInterface.hpp"
#include "strus/weightingFunctionInterface.hpp"
#include "strus/reference.hpp"
#include <vector>
#include <string>
#include <map>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Provides the objects needed for query processing
class QueryProcessor
	:public QueryProcessorInterface
{
public:
	/// \brief Constructor
	/// \param[in] errorhnd_ reference to error buffer (ownership hold by caller)
	explicit QueryProcessor( ErrorBufferInterface* errorhnd_);

	/// \brief Destructor
	virtual ~QueryProcessor();

	virtual void
		definePostingJoinOperator(
			const std::string& name,
			PostingJoinOperatorInterface* op);

	virtual const PostingJoinOperatorInterface*
		getPostingJoinOperator(
			const std::string& name) const;

	virtual void
		defineWeightingFunction(
			const std::string& name,
			WeightingFunctionInterface* func);

	virtual const WeightingFunctionInterface*
		getWeightingFunction(
			const std::string& name) const;

	virtual void
		defineSummarizerFunction(
			const std::string& name,
			SummarizerFunctionInterface* sumfunc);
	
	virtual const SummarizerFunctionInterface*
		getSummarizerFunction(
			const std::string& name) const;

	virtual const char* getDescription( FunctionType type, const std::string& name) const;

	virtual std::vector<std::string> getFunctionList( FunctionType type) const;

private:
	std::map<std::string,Reference<SummarizerFunctionInterface> > m_summarizers;
	std::map<std::string,Reference<WeightingFunctionInterface> > m_weighters;
	std::map<std::string,Reference<PostingJoinOperatorInterface> > m_joiners;
	ErrorBufferInterface* m_errorhnd;
};

}//namespace
#endif

