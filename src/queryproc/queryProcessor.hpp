/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
/// \brief Forward declaration
class FileLocatorInterface;

/// \brief Provides the objects needed for query processing
class QueryProcessor
	:public QueryProcessorInterface
{
public:
	/// \brief Constructor
	/// \param[in] errorhnd_ reference to error buffer (ownership hold by caller)
	QueryProcessor( const FileLocatorInterface* filelocator_, ErrorBufferInterface* errorhnd_);

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

	virtual std::vector<std::string>
		getFunctionList(
			const FunctionType& type) const;

	virtual void
		defineScalarFunctionParser(
			const std::string& name,
			ScalarFunctionParserInterface* parser);

	virtual const ScalarFunctionParserInterface*
		getScalarFunctionParser(
			const std::string& name) const;

	virtual std::string getResourceFilePath( const std::string& filename) const;

private:
	typedef std::map<std::string,Reference<SummarizerFunctionInterface> > SummarizerFunctionMap;
	typedef std::map<std::string,Reference<WeightingFunctionInterface> > WeightingFunctionMap;
	typedef std::map<std::string,Reference<PostingJoinOperatorInterface> > PostingJoinOperatorMap;
	typedef std::map<std::string,Reference<ScalarFunctionParserInterface> > ScalarFunctionParserMap;
	SummarizerFunctionMap m_summarizers;
	WeightingFunctionMap m_weighters;
	PostingJoinOperatorMap m_joiners;
	ScalarFunctionParserMap m_funcparsers;
	ErrorBufferInterface* m_errorhnd;
	const FileLocatorInterface* m_filelocator;
};

}//namespace
#endif

