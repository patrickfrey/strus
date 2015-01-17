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
	struct TermDef
	{
		TermDef( const TermDef& o)
			:set(o.set),type(o.type),value(o.value){}
		TermDef( const std::string& s, const std::string& t, const std::string& v)
			:set(s),type(t),value(v){}

		std::string set;	///< term set name
		std::string type;	///< term type name
		std::string value;	///< term value
	};

	struct WeightingFunctionDef
	{
		WeightingFunctionDef()
			:function(0),functionName(),parameters(),weightingSets(),selectorSets(){}
		WeightingFunctionDef( const WeightingFunctionDef& o)
			:function(o.function),functionName(o.functionName),parameters(o.parameters),weightingSets(o.weightingSets),selectorSets(o.selectorSets){}
		WeightingFunctionDef(
				const WeightingFunctionInterface* function_,
				const std::string& functionName_,
				const std::vector<ArithmeticVariant>& parameters_,
				const std::vector<std::string>& weightingSets_,
				const std::vector<std::string>& selectorSets_)
			:function(function_),functionName(functionName_),parameters(parameters_),weightingSets(weightingSets_),selectorSets(selectorSets_){}

		const WeightingFunctionInterface* function;	///< function used for weighting
		std::string functionName;			///< name of the function used for weighting
		std::vector<ArithmeticVariant> parameters;	///< weighting function parameters
		std::vector<std::string> weightingSets;		///< posting sets that are used for weighting
		std::vector<std::string> selectorSets;		///< posting sets selecting the documents to match
	};

	struct SummarizerDef
	{
		SummarizerDef( 
				const SummarizerFunctionInterface* function_,
				const std::string& functionName_,
				const std::vector<ArithmeticVariant>& parameters_,
				const std::string& resultAttribute_,
				const std::string& contentType_,
				const std::string& structSet_,
				const std::vector<std::string>& featureSet_)
			:function(function_)
			,functionName(functionName_)
			,parameters(parameters_)
			,resultAttribute(resultAttribute_)
			,contentType(contentType_)
			,structSet(structSet_)
			,featureSet(featureSet_){}

		SummarizerDef( const SummarizerDef& o)
			:function(o.function)
			,functionName(o.functionName)
			,parameters(o.parameters)
			,resultAttribute(o.resultAttribute)
			,contentType(o.contentType)
			,structSet(o.structSet)
			,featureSet(o.featureSet){}

		const SummarizerFunctionInterface* function;	///< summarization function
		std::string functionName;			///< name of the summarization function
		std::vector<ArithmeticVariant> parameters;	///< summarization function parameters
		std::string resultAttribute;			///< name of the result attribute the summarization is returned as
		std::string contentType;			///< content type to extract from the forward indes as result of summarization
		std::string structSet;				///< set of structure elements
		std::vector<std::string> featureSet;		///< set of features to seek for matches for summarization
	};

private:
	void parseWeightingFunctionDef( char const*& src);
	void parseTermDef( char const*& src);
	void parseSummarizeDef( char const*& src);
	void parseJoinOperationDef( char const*& src);
	void loadProgram( const std::string& source);

public:
	enum {MaxSizeFeatureSet=100};

	explicit QueryEval( const QueryProcessorInterface* processor_)
		:m_processor(processor_){}

	QueryEval( const QueryEval& o)
		:m_processor(o.m_processor)
		,m_weightingFunction(o.m_weightingFunction)
		,m_summarizers(o.m_summarizers)
		,m_predefinedTerms(o.m_predefinedTerms)
	{}
	QueryEval( const QueryProcessorInterface* processor_, const std::string& source);

	virtual QueryInterface* createQuery() const;

	const std::vector<TermDef>& predefinedTerms() const		{return m_predefinedTerms;}
	const std::vector<SummarizerDef>& summarizers() const		{return m_summarizers;}
	const WeightingFunctionDef& weightingFunction() const		{return m_weightingFunction;}

	virtual void print( std::ostream& out) const;

private:
	const QueryProcessorInterface* m_processor;

	WeightingFunctionDef m_weightingFunction;
	std::vector<SummarizerDef> m_summarizers;
	std::vector<TermDef> m_predefinedTerms;
};

}//namespace
#endif

