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
#ifndef _STRUS_WEIGHTING_BM25PFF_HPP_INCLUDED
#define _STRUS_WEIGHTING_BM25PFF_HPP_INCLUDED
#include "strus/weightingFunctionInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/weightingFunctionContextInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include "proximityWeightAccumulator.hpp"
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;

/// \class WeightingFunctionContextBM25pff
/// \brief Weighting function based on the BM25pff (BM25 with proximity feature frequency) formula
class WeightingFunctionContextBM25pff
	:public WeightingFunctionContextInterface
{
public:
	WeightingFunctionContextBM25pff(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		double k1_,
		double b_,
		unsigned int windowsize_,
		unsigned int cardinality_,
		double ffbase_,
		double mindf_,
		double avgDocLength_,
		double titleinc_,
		double nofCollectionDocuments_,
		const std::string& metadata_doclen_,
		const std::string& metadata_title_maxpos_,
		const std::string& metadata_title_size_,
		ErrorBufferInterface* errorhnd_);

	virtual void addWeightingFeature(
			const std::string& name_,
			PostingIteratorInterface* itr_,
			float weight_,
			const TermStatistics& stats_);

	virtual double call( const Index& docno);

private:
	enum {MaxNofArguments=64};				///< chosen to fit in a bitfield of 64 bits
	double m_k1;						///< k1 value of BM25
	double m_b;						///< b value of BM25
	unsigned int m_windowsize;				///< maximum position range of a window considered for weighting
	unsigned int m_cardinality;				///< minumum number of features in a window considered for weighting
	double m_ffbase;					///< constant base value of ff
	double m_maxdf;						///< the maximum df of features considered for proximity weighing as fraction of the total collection size
	double m_avgDocLength;					///< average document length in the collection
	double m_titleinc;					///< ff increment for title features
	double m_nofCollectionDocuments;			///< number of documents in the collection
	ProximityWeightAccumulator::WeightArray m_idfar;	///< array of idfs
	PostingIteratorInterface* m_itrar[ MaxNofArguments];	///< array if weighted features
	PostingIteratorInterface* m_structar[ MaxNofArguments];	///< array of end of structure elements
	PostingIteratorInterface* m_paraar[ MaxNofArguments];	///< array of end of paragraph elements
	std::size_t m_itrarsize;				///< number of weighted features
	std::size_t m_structarsize;				///< number of end of structure elements
	std::size_t m_paraarsize;				///< number of paragraph elements (now summary accross paragraph borders)
	std::size_t m_nof_maxdf_features;			///< number of features with a df bigger than maximum
	ProximityWeightAccumulator::WeightArray m_weightincr;	///< array of proportional weight increments 
	bool m_initialized;					///< true, if the structures have already been initialized
	MetaDataReaderInterface* m_metadata;			///< meta data reader
	int m_metadata_doclen;					///< meta data doclen handle
	int m_metadata_title_maxpos;				///< meta data title maximum position handle
	int m_metadata_title_size;				///< meta data title size
	ErrorBufferInterface* m_errorhnd;			///< buffer for error messages
};


/// \class WeightingFunctionInstanceBM25pff
/// \brief Weighting function instance based on the BM25pff formula
class WeightingFunctionInstanceBM25pff
	:public WeightingFunctionInstanceInterface
{
public:
	explicit WeightingFunctionInstanceBM25pff( ErrorBufferInterface* errorhnd_)
		:m_k1(1.5),m_b(0.75),m_avgdoclen(1000),m_windowsize(100),m_cardinality(0),m_ffbase(0.4),m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionInstanceBM25pff(){}

	virtual void addStringParameter( const std::string& name, const std::string& value);
	virtual void addNumericParameter( const std::string& name, const ArithmeticVariant& value);

	virtual WeightingFunctionContextInterface* createFunctionContext(
			const StorageClientInterface* storage_,
			MetaDataReaderInterface* metadata,
			const GlobalStatistics& stats) const;

	virtual std::string tostring() const;

private:
	double m_k1;					///< BM25 k1 parameter
	double m_b;					///< BM25 b parameter
	double m_avgdoclen;				///< average document length
	double m_titleinc;				///< ff increment for title features
	std::string m_metadata_doclen;			///< attribute defining the document length
	std::string m_metadata_title_maxpos;		///< (optional) meta data element defining the last title position
	std::string m_metadata_title_size;		///< (optional) meta data element defining the size of the title
	unsigned int m_windowsize;			///< size of window for proximity weighting
	unsigned int m_cardinality;			///< minimal number of query features in a window
	double m_ffbase;				///< base used for feature frequency calculation
	double m_maxdf;					///< the maximum df of features considered for proximity weighing as fraction of the total collection size
	ErrorBufferInterface* m_errorhnd;		///< buffer for error messages
};


/// \class WeightingFunctionBM25pff
/// \brief Weighting function based on the BM25pff formula
class WeightingFunctionBM25pff
	:public WeightingFunctionInterface
{
public:
	explicit WeightingFunctionBM25pff( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~WeightingFunctionBM25pff(){}

	virtual WeightingFunctionInstanceInterface* createInstance() const;

	virtual Description getDescription() const;

private:
	ErrorBufferInterface* m_errorhnd;				///< buffer for error messages
};

}//namespace
#endif

