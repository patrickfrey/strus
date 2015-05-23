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
#ifndef _STRUS_WEIGHTING_BM25_DPFC_HPP_INCLUDED
#define _STRUS_WEIGHTING_BM25_DPFC_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingExecutionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace strus
{

/// \class WeightingExecutionContextBM25_dpfc
/// \brief Weighting function based on the BM25 formula with an artificial ff calculated from the real ff and some discrete increments given by some feature occurrence relation checks based on proximity and structures (sentences) 
class WeightingExecutionContextBM25_dpfc
	:public WeightingExecutionContextInterface
{
public:
	WeightingExecutionContextBM25_dpfc(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		float k1_,
		float b_,
		float avgDocLength_,
		const std::string& attribute_content_doclen_,
		const std::string& attribute_title_doclen_,
		unsigned int proximityMinDist_,
		float title_ff_incr,
		float sequence_ff_incr,
		float sentence_ff_incr);

	struct Feature
	{
		PostingIteratorInterface* itr;
		float weight;
		float idf;

		Feature( PostingIteratorInterface* itr_, float weight_, float idf_)
			:itr(itr_),weight(weight_),idf(idf_){}
		Feature( const Feature& o)
			:itr(o.itr),weight(o.weight),idf(o.idf){}
	};

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			float weight_);

	virtual float call( const Index& docno);

private:
	float m_k1;
	float m_b;
	float m_avgDocLength;
	float m_nofCollectionDocuments;
	std::vector<Feature> m_weight_featar;
	std::vector<Feature> m_struct_featar;
	PostingIteratorInterface* m_title_itr;
	MetaDataReaderInterface* m_metadata;
	int m_metadata_content_doclen;
	int m_metadata_title_doclen;
	unsigned int m_proximityMinDist;
	float m_title_ff_incr;
	float m_sequence_ff_incr;
	float m_sentence_ff_incr;
};

/// \class WeightingFunctionInstanceBM25_dpfc
/// \brief Weighting function instance based on the BM25 formula with an artificial ff calculated from the real ff and some discrete increments given by some feature occurrence relation checks based on proximity and structures (sentences) 
class WeightingFunctionInstanceBM25_dpfc
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25_dpfc()
		:m_b(0.75),m_k1(1.5),m_avgdoclen(1000),m_proximityMinDist(150),m_title_ff_incr(1.5),m_sequence_ff_incr(1.5),m_sentence_ff_incr(0.5)
	{}

	virtual ~WeightingFunctionInstanceBM25_dpfc(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual WeightingExecutionContextInterface* createExecutionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata) const
	{
		return new WeightingExecutionContextBM25_dpfc( storage_, metadata, m_b, m_k1, m_avgdoclen, m_attribute_content_doclen, m_attribute_title_doclen, m_proximityMinDist, m_title_ff_incr, m_sequence_ff_incr, m_sentence_ff_incr);
	}

	virtual std::string tostring() const
	{
		std::ostringstream rt;
		rt << std::setw(2) << std::setprecision(5)
			<< "b=" << m_b << ", k1=" << m_k1 << ", avgdoclen=" << m_avgdoclen << ", doclen=" << m_attribute_content_doclen << ", doclen_title=" << m_attribute_title_doclen << ", proxmindist=" << m_proximityMinDist << ", titleinc=" << m_title_ff_incr << ", seqinc=" << m_sequence_ff_incr << "strinc=" << m_sentence_ff_incr << std::endl;
		return rt.str();
	}

private:
	float m_b;
	float m_k1;
	float m_avgdoclen;
	std::string m_attribute_content_doclen;
	std::string m_attribute_title_doclen;
	unsigned int m_proximityMinDist;
	float m_title_ff_incr;
	float m_sequence_ff_incr;
	float m_sentence_ff_incr;
};


/// \class WeightingFunctionBM25_dpfc
/// \brief Weighting function based on the BM25 formula with an artificial ff calculated from the real ff and some discrete increments given by some feature occurrence relation checks based on proximity and structures (sentences) 
class WeightingFunctionBM25_dpfc
	:public WeightingFunctionInterface
{
public:
	WeightingFunctionBM25_dpfc(){}

	virtual ~WeightingFunctionBM25_dpfc(){}

	virtual WeightingFunctionInstanceInterface* createInstance() const
	{
		return new WeightingFunctionInstanceBM25_dpfc();
	}
};

}//namespace
#endif

