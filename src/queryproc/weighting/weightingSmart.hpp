/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
///\file weightingSmart.hpp
///\brief Implementation of a weighting function defined as function on tf,df,N and some metadata references in a string
#ifndef _STRUS_WEIGHTING_SMART_HPP_INCLUDED
#define _STRUS_WEIGHTING_SMART_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/scalarFunctionInterface.hpp"
#include "strus/reference.hpp"
#include "strus/index.hpp"
#include "strus/base/math.hpp"
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

/// \class WeightingFunctionContextSmart
/// \brief Context of a weighting function defined as function on tf,df,N and some metadata references in a string
/// \note The name Smart is taken from the SMART weighting schemes (https://en.wikipedia.org/wiki/SMART_Information_Retrieval_System)
class WeightingFunctionContextSmart
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextSmart(
		const ScalarFunctionInterface* func_,
		MetaDataReaderInterface* metadata_,
		const std::vector<Index>& metadatahnd_,
		double nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_);

	class Feature
	{
	public:
		Feature( const Feature& o)
			:m_itr(o.m_itr),m_weight(o.m_weight),m_df(o.m_df){}
		Feature( PostingIteratorInterface* itr_, double weight_, const TermStatistics& stats_)
			:m_itr(itr_),m_weight(weight_)
			,m_df(stats_.documentFrequency()>=0?stats_.documentFrequency():std::numeric_limits<double>::quiet_NaN()){}

		double df() const
		{
			if (strus::Math::isnan(m_df))
			{
				m_df = m_itr->documentFrequency();
			}
			return m_df;
		}
		double ff() const
		{
			return m_itr->frequency();
		}
		double weight() const
		{
			return m_weight;
		}
		Index skipDoc( Index docno)
		{
			return m_itr->skipDoc( docno);
		}

	private:
		PostingIteratorInterface* m_itr;
		double m_weight;
		mutable double m_df;
	};

public:
	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			double weight_,
			const TermStatistics& stats_);

	virtual void setVariableValue( const std::string& name_, double value);

	virtual double call( const Index& docno);

	virtual std::string debugCall( const Index& docno);

public:
	enum {MaxNofParameter=64};				///< maximum number of arguments passed to the defined function

private:
	void fillParameter( const Index& docno, double ff, double df, double* parameterVector) const;

private:
	typedef std::vector<Feature> FeatureVector;
	Reference<ScalarFunctionInstanceInterface> m_func;	///< scalar function instance to execute
	std::vector<Feature> m_featar;				///< list of argument features
	Reference<MetaDataReaderInterface> m_metadata;		///< meta data reader
	std::vector<Index> m_metadatahnd;			///< array of meta data element handles feeded to the function
	double m_nofCollectionDocuments;			///< document collection size
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class WeightingFunctionInstanceSmart
/// \brief Instance of a weighting function defined as function on tf,df,N and some metadata references in a string
class WeightingFunctionInstanceSmart
	:public WeightingFunctionInstanceInterface
{
public:
	WeightingFunctionInstanceSmart(
			const QueryProcessorInterface* queryproc_,
			ErrorBufferInterface* errorhnd_)
		:m_queryproc(queryproc_)
		,m_parser(0)
		,m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceSmart(){}

	virtual void addStringParameter( const std::string& name_, const std::string& value);
	virtual void addNumericParameter( const std::string& name_, const NumericVariant& value);

	virtual std::vector<std::string> getVariables() const;

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			const GlobalStatistics& stats) const;

	virtual const char* name() const	{return "smart";}
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


/// \class WeightingFunctionSmart
/// \brief Weighting function defined as function on tf,df,N and some metadata references in a string
class WeightingFunctionSmart
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionSmart( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionSmart(){}

	virtual WeightingFunctionInstanceInterface* createInstance(
			const QueryProcessorInterface* processor) const;

	virtual const char* name() const	{return "smart";}
	virtual StructView view() const;

private:
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};

}//namespace
#endif

