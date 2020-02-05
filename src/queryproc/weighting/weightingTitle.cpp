/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "weightingTitle.hpp"
#include "postingIteratorHelpers.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "strus/constants.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/math.hpp"
#include "strus/base/bitset.hpp"
#include "viewUtils.hpp"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <limits>

using namespace strus;
#define THIS_METHOD_NAME const_cast<char*>("title")
#undef STRUS_LOWLEVEL_DEBUG

WeightingFunctionContextTitle::WeightingFunctionContextTitle(
		const StorageClientInterface* storage_,
		double hierarchyWeightFactor_,
		int maxNofResults_,
		double maxdf_,
		double nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_structitr(storage_->createStructIterator()),m_postingarsize(0)
	,m_hierarchyWeightFactor(hierarchyWeightFactor_)
	,m_maxdf(maxdf_),m_maxNofResults(maxNofResults_),m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_lastResult(),m_errorhnd(errorhnd_)
{
	std::memset( &m_isStopWordAr, false, sizeof( m_isStopWordAr));
	m_hierarchyWeight[ 0] = 1.0;
	for (int li=1; li < strus::Constants::MaxStructLevels; ++li)
	{
		m_hierarchyWeight[ li] = m_hierarchyWeight[ li-1] * m_hierarchyWeightFactor;
	}
	if (!m_structitr.get()) throw std::runtime_error(_TXT("failed to create structure iterator"));
}

void WeightingFunctionContextTitle::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr,
		double weight,
		const TermStatistics& termstats)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match"))
		{
			double nofMatches = termstats.documentFrequency()>=0?termstats.documentFrequency():itr->documentFrequency();
			if (!strus::Math::isequal( weight, 0.0) && !strus::Math::isequal( weight, 1.0))
			{
				m_errorhnd->info( _TXT("warning: weight ignored for 'title' weighting method"));
			}
			if (m_postingarsize > MaxNofArguments) throw strus::runtime_error( _TXT("number of weighting features (%d) out of range"), m_postingarsize);
			
			m_postingar[ m_postingarsize] = itr;
			m_isStopWordAr[ m_postingarsize] = (nofMatches > m_nofCollectionDocuments * m_maxdf);
			++m_postingarsize;
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' weighting function feature parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding weighting feature to '%s' weighting: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionContextTitle::setVariableValue( const std::string& name_, double value)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), THIS_METHOD_NAME);
}

struct StructureReference
{
	int level;
	int structindex;
	strus::IndexRange field;

	StructureReference()
		:level(0),structindex(0),field(){}
	StructureReference( int level_, int structindex_, const strus::IndexRange& field_)
		:level(level_),structindex(structindex_),field(field_)
	{}
	StructureReference( const StructureReference& o)
		:level(o.level),structindex(o.structindex),field(o.field)
	{}
	bool operator < (const StructureReference& o) const
	{
		return (level == o.level) 
			? structindex == o.structindex
				? field < o.field
				: structindex < o.structindex
			: level < o.level;
	}
};

typedef std::pair<int,int> QueryPostingRange;

typedef StructureReference StructureContentReference;

struct StructureHeaderReference
	:public StructureReference
{
	QueryPostingRange queryPostingRange;
	int hierarchy;

	StructureHeaderReference( const StructureReference& stu, const QueryPostingRange& queryPostingRange_, int hierarchy_)
		:StructureReference(stu),queryPostingRange(queryPostingRange_),hierarchy(hierarchy_)
	{}
	StructureHeaderReference( const StructureHeaderReference& o)
		:StructureReference(o),queryPostingRange(o.queryPostingRange),hierarchy(o.hierarchy)
	{}
	bool operator < (const StructureHeaderReference& o) const
	{
		return
			hierarchy == o.hierarchy
				? (level == o.level) 
					? structindex == o.structindex
						? field == o.field
							? queryPostingRange.second == o.queryPostingRange.second
								? queryPostingRange.first < o.queryPostingRange.first
								: queryPostingRange.second < o.queryPostingRange.second
							: field < o.field
						: structindex < o.structindex
					: level < o.level
				: hierarchy < o.hierarchy;
	}
	bool completeMatch() const
	{
		return (int)(queryPostingRange.second - queryPostingRange.first) == (int)field.len();
	}
};

int WeightingFunctionContextTitle::tryQuerySequenceMatchToField( int postingIdx, const strus::IndexRange& field)
{
	int rt = 0;
	int fi = 0, fe = field.len();
	int pi = postingIdx, pe = postingIdx + field.len();
	if (pe > m_postingarsize) pe = m_postingarsize;
	for (; fi != fe && pi != pe; ++fi,++pi)
	{
		strus::Index expectpos = field.start()+fi;
		if (m_postingar[ pi]->skipPos( expectpos) == expectpos)
		{
			++rt;
		}
		else
		{
			break;
		}
	}
	return rt;
}

struct StructureCoverSearch
{
	strus::bitset<WeightingFunctionContextTitle::MaxNofArguments> postingsUsed;
	strus::IndexRange field;
	int hierarchy;
	double weight;

	StructureCoverSearch()
		:postingsUsed(),field(),hierarchy(0),weight(0.0){}
	StructureCoverSearch( const strus::IndexRange& field_, int hierarchy_, double weight_, const QueryPostingRange& postingsUsed_)
		:postingsUsed(),field(field_),hierarchy(hierarchy_),weight(weight_)
	{
		int pi = postingsUsed_.first, pe = postingsUsed_.second;
		for (; pi != pe; ++pi)
		{
			postingsUsed.set( pi, true);
		}
	}
	StructureCoverSearch( const StructureCoverSearch& o)
		:postingsUsed(o.postingsUsed),field(o.field),hierarchy(o.hierarchy),weight(o.weight){}

	bool operator < (const StructureCoverSearch& o) const
	{
		return hierarchy == o.hierarchy
			? weight == o.weight
				? field == o.field
					? postingsUsed < o.postingsUsed
					: field > o.field
				: weight > o.weight
			: hierarchy < o.hierarchy;
	}
	bool overlapsPostingsUsed( const QueryPostingRange& postingsUsed_)
	{
		int pi = postingsUsed_.first, pe = postingsUsed_.second;
		for (; pi != pe; ++pi)
		{
			if (postingsUsed.test( pi)) return true;
		}
		return false;
	}
};

const std::vector<WeightedField>& WeightingFunctionContextTitle::call( const Index& docno)
{
	try
	{
		typedef std::set<int> StructIndexHeaderSet;
		StructIndexHeaderSet structIndexHeaderSet;

		typedef std::multimap<int,int> StructIndexContentMap;
		typedef std::pair<StructIndexContentMap::const_iterator,StructIndexContentMap::const_iterator> StructIndexContentMapRange;
		StructIndexContentMap structIndexContentMap;

		std::vector<StructureHeaderReference> headerar;
		std::vector<StructureContentReference> contentar;
		headerar.reserve( 256);
		contentar.reserve( 1024);

		m_lastResult.resize( 0);
		m_structitr->skipDoc(docno);

		// [1] Fill structure reference candidate arrays:
		int pi = 0, pe = m_postingarsize;
		for (; pi != pe; ++pi)
		{
			if (m_postingar[ pi]->skipDoc( docno) != docno) return m_lastResult;
		}
		pi = 0;
		for (; pi != pe; ++pi)
		{
			int li=0, le = m_structitr->levels();
			for (; li != le; ++li)
			{
				strus::Index posno = m_postingar[ pi]->skipPos( 0);
				for (; posno; posno = m_postingar[ pi]->skipPos( posno))
				{
					strus::IndexRange field = m_structitr->skipPos( li, posno);
					if (!field.defined()) break;
					if (field.contain( posno))
					{
						StructureLinkArray lnkar = m_structitr->links( li);
						int si=0, se = lnkar.nofLinks();
						for (; si != se; ++si)
						{
							const StructureLink& lnk = lnkar[ si];
							StructureReference stu( li/*level*/, lnk.index(), field);
							if (lnk.header())
							{
								if (field.len() <= MaxNofArguments)
								{
									structIndexHeaderSet.insert( lnk.index());
									int matchlen = tryQuerySequenceMatchToField( pi, field);
									if (matchlen > 0)
									{
										for (int submatchlen=1; submatchlen<=matchlen; ++submatchlen)
										{
											QueryPostingRange prange( pi, pi+submatchlen);
											headerar.push_back( StructureHeaderReference( stu, prange, 0));
										}
									}
								}
							}
							else
							{
								contentar.push_back( stu);
							}
						}
						posno = field.end();
					}
					else
					{
						posno = field.start();
					}
				}
			}
		}
		{
			// [1.2] Sort content array and eliminate duplicates:
			std::sort( contentar.begin(), contentar.end());

			std::vector<StructureContentReference>::iterator
				ci = contentar.begin(), ce = contentar.end(), cp = contentar.begin();
			while (ci != ce)
			{
				*cp = *ci;
				for (++ci; ci != ce && ci->structindex == cp->structindex && ci->field.start() == cp->field.start(); ++ci){}
				++cp;
			}
			contentar.resize( cp - contentar.begin());
		}
		{
			// [1.3] Calculate the number of times a header element is covered by content
			//	field of another structure than itself corresponding to the position
			//	in the hierarchy:
			std::vector<StructureContentReference>::iterator
				ci = contentar.begin(), ce = contentar.end();
			for (; ci != ce; ++ci)
			{
				std::vector<StructureHeaderReference>::iterator hi = headerar.begin(), he = headerar.end();
				for (; hi != he; ++hi)
				{
					if (hi->structindex != ci->structindex && ci->field.cover( hi->field))
					{
						++hi->hierarchy;
					}
				}
			}
			// [NOTE 2020-1-27] Maybe the hierarchy info could be stored in the block.
			//	If it is used elsewhere too, we could think about storing it.
			//	If not the bits wasted per structure are not worth it.
		}
		{
			// [1.4] Sort Headers by hierarchy:
			std::sort( headerar.begin(), headerar.end());
		}
		{
			// [1.5] Eliminate unused content elements and build a map structure index to 
			//	content element:
			std::vector<StructureContentReference>::iterator
				ci = contentar.begin(), ce = contentar.end();
			for (; ci != ce && structIndexHeaderSet.find( ci->structindex) != structIndexHeaderSet.end(); ++ci)
			{}
			if (ci != ce)
			{
				std::vector<StructureContentReference>::iterator cn = ci;
				for (++cn; cn != ce; ++cn)
				{
					if (structIndexHeaderSet.find( cn->structindex) != structIndexHeaderSet.end())
					{
						*ci = *cn;
						++ci;
					}
				}
				contentar.resize( ci - contentar.begin());
			}
			ci = contentar.begin(), ce = contentar.end();
			int cidx = 0;
			for (; ci != ce; ++ci,++cidx)
			{
				structIndexContentMap.insert( std::pair<int,int>( ci->structindex, cidx));
			}
		}
		// [2] Calculate the queue for searching complete title path solutions:
		std::set<StructureCoverSearch> queue;
		{
			std::vector<StructureHeaderReference>::iterator
				ei = headerar.begin(), ee = headerar.end();
			for (; ei != ee; ++ei)
			{
				int consumedPostings = ei->queryPostingRange.second - ei->queryPostingRange.first;
				double weight = (double)(consumedPostings) / (double)m_postingarsize;

				if (ei->hierarchy == 0)
				{
					// ... initial field must be top level and not coverd by any other field
					if (consumedPostings == m_postingarsize)
					{
						// ... all query features used
						if (strus::Math::abs( weight - 1.0) > std::numeric_limits<float>::epsilon()) throw std::runtime_error(_TXT("logic error in weight calculation"));
						m_lastResult.push_back( WeightedField( ei->field, weight));
						if (m_maxNofResults && m_maxNofResults == (int)m_lastResult.size())
						{
							queue.clear();
							break;
						}
					}
					else if (ei->completeMatch())
					{
						// ... not all query features used but got complete match of the title
						StructIndexContentMapRange crg = structIndexContentMap.equal_range( ei->structindex);
						StructIndexContentMap::const_iterator ci = crg.first, ce = crg.second;
						for (; ci != ce; ++ci)
						{
							const StructureContentReference& content = contentar[ ci->second];
							queue.insert( StructureCoverSearch( content.field, ei->hierarchy, weight, ei->queryPostingRange));
						}
					}
				}
			}
		}
		// [3] Process the queue until no more candidates left
		while (!queue.empty())
		{
			StructureCoverSearch search = *queue.begin();
			queue.erase( queue.begin());

			std::vector<StructureHeaderReference>::iterator hi = headerar.begin(), he = headerar.end();
			for (; hi != he; ++hi)
			{
				if (hi->hierarchy > search.hierarchy && search.field.cover( hi->field) && !search.overlapsPostingsUsed( hi->queryPostingRange))
				{
					int consumedPostings = hi->queryPostingRange.second - hi->queryPostingRange.first;
					double weight = search.weight
							+ (((double)(consumedPostings) / (double)m_postingarsize)
								* m_hierarchyWeight[ hi->hierarchy]);

					if (consumedPostings + (int)search.postingsUsed.size() == m_postingarsize)
					{
						bool isValidPartialMatch = false;
						if (hi->completeMatch())
						{
							// ... all query features used
							isValidPartialMatch = true;
						}
						else
						{
							int qi = hi->queryPostingRange.first, qe = hi->queryPostingRange.second;
							for (; qi != qe && m_isStopWordAr[ qi]; ++qi){}
							if (qi != qe)
							{
								//... at least one non stopword used in a partial match
								isValidPartialMatch = true;
							}
						}
						if (isValidPartialMatch)
						{
							m_lastResult.push_back( WeightedField( hi->field, weight));
							if (m_maxNofResults && m_maxNofResults == (int)m_lastResult.size()) break;
						}
					}
					else if (hi->completeMatch())
					{
						// ... not all query features used but complete match of the title
						StructureCoverSearch newsearch( strus::IndexRange(), hi->hierarchy, weight, hi->queryPostingRange);
						newsearch.postingsUsed.join( search.postingsUsed);

						StructIndexContentMapRange crg = structIndexContentMap.equal_range( hi->structindex);
						StructIndexContentMap::const_iterator ci = crg.first, ce = crg.second;
						for (; ci != ce; ++ci)
						{
							newsearch.field = contentar[ ci->second].field;
							queue.insert( newsearch);
						}
					}
				}
			}
		}
		return m_lastResult;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error calling weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, m_lastResult);
}

static NumericVariant parameterValue( const std::string& name_, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name_.c_str(), value.c_str());
	return rt;
}

void WeightingFunctionInstanceTitle::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature"), name_.c_str(), THIS_METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name_, "hf"))
		{
			addNumericParameter( name_, parameterValue( name_, value));
		}
		else
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("unknown parameter '%s' for weighting scheme '%s'"), name_.c_str(), THIS_METHOD_NAME);
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' weighting function add string parameter: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void WeightingFunctionInstanceTitle::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as feature"), name_.c_str(), THIS_METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name_, "struct"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined as string"), name_.c_str(), THIS_METHOD_NAME);
		}
		else if (strus::caseInsensitiveEquals( name_, "hf"))
		{
			double val = (double)value;
			if (val < 0.0 || val > 1.0)
			{
				m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to be defined a floating point number between 0.0 and 1.0"), name_.c_str(), THIS_METHOD_NAME);
			}
			else
			{
				m_hierarchyWeightFactor = val;
			}
		}
		else if (strus::caseInsensitiveEquals( name_, "results"))
		{
			int N = value.toint();
			if (N >= 0)
			{
				m_maxNofResults = N;
			}
			else
			{
				m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("positive number expected as argument"));
			}
		}
		else if (strus::caseInsensitiveEquals( name_, "maxdf"))
		{
			m_maxdf = (double)value;
			if (m_maxdf < 0.0 || m_maxdf > 1.0)
			{
				m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for weighting scheme '%s' expected to a positive floating point number between 0.0 and 1.0"), name_.c_str(), THIS_METHOD_NAME);
			}
		}
		else
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("unknown parameter '%s' for weighting scheme '%s'"), name_.c_str(), THIS_METHOD_NAME);
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error '%s' weighting function add numeric parameter: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

WeightingFunctionContextInterface* WeightingFunctionInstanceTitle::createFunctionContext(
			const StorageClientInterface* storage,
			const GlobalStatistics& stats) const
{
	try
	{
		GlobalCounter nofdocs = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage->nofDocumentsInserted();
		return new WeightingFunctionContextTitle( storage, m_hierarchyWeightFactor, m_maxNofResults, m_maxdf, (double)nofdocs, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' weighting function: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionInstanceTitle::view() const
{
	try
	{
		StructView rt;
		rt( "hf", m_hierarchyWeightFactor);
		if (m_maxNofResults)
		{
			rt( "results", m_maxNofResults);
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' weighting function introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}

WeightingFunctionInstanceInterface* WeightingFunctionTitle::createInstance(
			const QueryProcessorInterface* processor) const
{
	return new WeightingFunctionInstanceTitle( m_errorhnd);
}

StructView WeightingFunctionTitle::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Calculate the document weight based on features appearing in the title or headings only"));
		rt( P::Feature, "match", _TXT( "defines the query features to weight"), "");
		rt( P::Feature, "struct", _TXT( "defines the name of the structures used for determining titles"), "");
		rt( P::Numeric, "hf", _TXT("hierarchy weight factor (defines the weight of a heading against the weight of its enclosing title or heading, e.g. the weight loss of a subsection title against a section title"), "0.0:1.0");
		rt( P::Numeric, "results", _TXT("maximum number of results of 0 for unlimited"), "0..");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

