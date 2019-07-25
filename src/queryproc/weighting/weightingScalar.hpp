/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\file weightingScalar.hpp
///\brief Implementation of a weighting function defined as function on variables, constants and document metadata in a string
#ifndef _STRUS_WEIGHTING_SCALAR_HPP_INCLUDED
#define _STRUS_WEIGHTING_SCALAR_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/scalarFunctionInterface.hpp"
#include "strus/reference.hpp"
#include "strus/index.hpp"
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class QueryProcessorInterface;
/// \brief Forward declaration
class ScalarFunctionInstanceInterface;
/// \brief Forward declaration
class ScalarFunctionParserInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class PostingIteratorInterface;

/// \class WeightingFunctionContextScalar
/// \brief Context of a weighting function defined as a function on constants, variables and document metadata in a string
class WeightingFunctionContextScalar
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextScalar(
		const ScalarFunctionInterface* func_,
		MetaDataReaderInterface* metadata_,
		const std::vector<Index>& metadatahnd_,
		double nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_);

public:
	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			double weight_,
			const TermStatistics& stats_);

	virtual void setVariableValue( const std::string& name, double value);

	virtual double call( const Index& docno);

	virtual std::string debugCall( const Index& docno);

public:
	enum {MaxNofParameter=64};				///< maximum number of arguments passed to the defined function

private:
	void fillParameter( double* parameterVector) const;

private:
	Reference<ScalarFunctionInstanceInterface> m_func;	///< scalar function instance to execute
	MetaDataReaderInterface* m_metadata;			///< meta data reader
	std::vector<Index> m_metadatahnd;			///< array of meta data element handles feeded to the function
	double m_nofCollectionDocuments;			///< document collection size
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class WeightingFunctionInstanceScalar
/// \brief Instance of a weighting function defined as a function on constants, variables and document metadata in a string
class WeightingFunctionInstanceScalar
	:public WeightingFunctionInstanceInterface
{
public:
	WeightingFunctionInstanceScalar(
			const QueryProcessorInterface* queryproc_,
			ErrorBufferInterface* errorhnd_)
		:m_queryproc(queryproc_)
		,m_parser(0)
		,m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceScalar(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const;

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const;

	virtual StructView view() const;

private:
	void initFunction() const;

private:
	std::string m_expression;
	const QueryProcessorInterface* m_queryproc;		///< query processor
	mutable const ScalarFunctionParserInterface* m_parser;	///< scalar function expression parser
	mutable Reference<ScalarFunctionInterface> m_func;	///< scalar function to execute
	std::vector<std::pair<std::string,double> > m_paramar;	///< variable definitions for scalar function
	std::vector<std::string> m_metadataar;			///< array of meta data elements to feed as arguments to scalar function
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class WeightingFunctionScalar
/// \brief Weighting function defined as a function on constants, variables and document metadata  in a string
class WeightingFunctionScalar
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionScalar( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionScalar(){}

	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual FunctionDescription getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};

}//namespace
#endif

