/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#ifndef _STRUS_WEIGHTING_BM25_HPP_INCLUDED
#define _STRUS_WEIGHTING_BM25_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class WeightingFunctionContextBM25
/// \brief Weighting function based on the BM25 formula
class WeightingFunctionContextBM25
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextBM25(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		float k1_,
		float b_,
		float avgDocLength_,
		float nofCollectionDocuments_,
		const std::string& attribute_doclen_,
		ErrorBufferInterface* errorhnd_);

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
			float weight_,
			const TermStatistics& stats_);

	virtual float call( const Index& docno);

private:
	float m_k1;
	float m_b;
	float m_avgDocLength;
	float m_nofCollectionDocuments;
	std::vector<Feature> m_featar;
	MetaDataReaderInterface* m_metadata;
	int m_metadata_doclen;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class WeightingFunctionInstanceBM25
/// \brief Weighting function instance based on the BM25 formula
class WeightingFunctionInstanceBM25
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25( ErrorBufferInterface* errorhnd_)
		:m_b(0.75),m_k1(1.5),m_avgdoclen(1000),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceBM25(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const;

	virtual std::string tostring() const;

private:
	float m_b;
	float m_k1;
	float m_avgdoclen;
	std::string m_attribute_doclen;
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};


/// \class WeightingFunctionBM25
/// \brief Weighting function based on the BM25 formula
class WeightingFunctionBM25
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionBM25( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionBM25(){}

	virtual WeightingFunctionInstanceInterface* createInstance() const;

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

