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
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		const std::vector<std::string>& m_metadataelemar,
		const ScalarFunctionInstanceInterface* func_,
		ErrorBufferInterface* errorhnd_);

	class Feature
	{
	public:
		Feature( const Feature& o)
			:m_itr(o.m_itr),m_weight(o.m_weight),m_df(o.m_df){}
		Feature( PostingIteratorInterface* itr_, double weight_, const TermStatistics& stats_)
			:m_itr(itr_),m_weight(weight_),m_df(stats_.documentFrequency()>=0?stats_.documentFrequency():std::numeric_limits<double>::quiet_NaN()){}

		double df() const
		{
			if (std::isnan(m_df))
			{
				m_df = m_itr->documentFrequency();
			}
			return m_df;
		}
		double ff() const
		{
			return m_match?m_itr->frequency():0.0;
		}
		double weight() const
		{
			return m_match?m_weight:0.0;
		}
		double match() const
		{
			return m_match?1.0:0.0;
		}

		void skipDoc( Index docno)
		{
			m_match = (docno == m_itr->skipDoc( docno));
		}

	private:
		PostingIteratorInterface* m_itr;
		double m_weight;
		mutable double m_df;
		bool m_match;
	};

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			float weight_,
			const TermStatistics& stats_);

	virtual double call( const Index& docno);

private:
	typedef std::vector<Feature> FeatureVector;
	std::vector<FeatureVector> m_featar;		///< list of argument features
	MetaDataReaderInterface* m_metadata;		///< meta data reader
	std::vector<Index> m_metadatahndar;		///< array of meta data element handles feeded to the function
	double m_collsize;				///< document collection size
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};


/// \class WeightingFunctionInstanceSmart
/// \brief Instance of a weighting function defined as function on tf,df,N and some metadata references in a string
class WeightingFunctionInstanceSmart
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceSmart( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceSmart(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const NumericVariant& value);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const;

	virtual std::string tostring() const;

private:
	std::string m_func;				///< scalar function string
	std::map<std::string,double> m_paramar;		///< variable definitions for scalar function
	std::string m_metadataar;			///< array of meta data elements to feed as arguments to scalar function
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
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

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

