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
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_structitr(storage_->createStructIterator()),m_postingarsize(0)
	,m_hierarchyWeightFactor(hierarchyWeightFactor_)
	,m_maxNofResults(maxNofResults_)
	,m_lastResult(),m_errorhnd(errorhnd_)
{
	m_levelWeight[ 0] = 1.0;
	for (int li=1; li < strus::Constants::MaxStructLevels; ++li)
	{
		m_levelWeight[ li] = m_levelWeight[ li-1] * m_hierarchyWeightFactor;
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
			if (!strus::Math::isequal( weight, 0.0) && !strus::Math::isequal( weight, 1.0))
			{
				m_errorhnd->info( _TXT("warning: weight ignored for 'title' weighting method"));
			}
			if (m_postingarsize > MaxNofArguments) throw strus::runtime_error( _TXT("number of weighting features (%d) out of range"), m_postingarsize);
			m_postingar[ m_postingarsize++] = Posting( itr);
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

	StructureHeaderReference( const StructureReference& stu, const QueryPostingRange& queryPostingRange_)
		:StructureReference(stu),queryPostingRange(queryPostingRange_)
	{}
	StructureHeaderReference( const StructureHeaderReference& o)
		:StructureReference(o),queryPostingRange(o.queryPostingRange)
	{}
	bool operator < (const StructureHeaderReference& o) const
	{
		return (level == o.level) 
			? structindex == o.structindex
				? field == o.field
					? queryPostingRange.second == o.queryPostingRange.second
						? queryPostingRange.first < o.queryPostingRange.first
						: queryPostingRange.second < o.queryPostingRange.second
					: field < o.field
				: structindex < o.structindex
			: level < o.level;
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
		if (m_postingar[ pi].iterator->skipPos( expectpos) == expectpos)
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
	int level;
	double weight;

	StructureCoverSearch()
		:postingsUsed(),field(),level(0),weight(0.0){}
	StructureCoverSearch( const strus::IndexRange& field_, int level_, double weight_, const QueryPostingRange& postingsUsed_)
		:postingsUsed(),field(field_),level(level_),weight(weight_)
	{
		int pi = postingsUsed_.first, pe = postingsUsed_.second;
		for (; pi != pe; ++pi)
		{
			postingsUsed.set( pi, true);
		}
	}
	StructureCoverSearch( const StructureCoverSearch& o)
		:postingsUsed(o.postingsUsed),field(o.field),level(o.level),weight(o.weight){}

	bool operator < (const StructureCoverSearch& o) const
	{
		return level == o.level
			? weight == o.weight
				? field == o.field
					? postingsUsed < o.postingsUsed
					: field > o.field
				: weight > o.weight
			: level < o.level;
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
			int li=0, le = m_structitr->levels();
			for (; li != le; ++li)
			{
				strus::Index posno = m_postingar[ pi].iterator->skipPos( 0);
				for (; posno; posno = m_postingar[ pi].iterator->skipPos( posno))
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
										QueryPostingRange prange( pi, pi+matchlen);
										headerar.push_back( StructureHeaderReference( stu, prange));
									}
								}
							}
							else
							{
								contentar.push_back( stu);
							}
						}
					}
					posno = field.end();
				}
			}
			// [1.2] Sort Headers by level:
			std::sort( headerar.begin(), headerar.end());
		}
		{
			// [1.3] Eliminate unused content elements and build map of structure index to content:
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
			std::sort( contentar.begin(), contentar.end());

			ci = contentar.begin(), ce = contentar.end();
			int cidx = 0;
			for (; ci != ce; ++ci,++cidx)
			{
				structIndexContentMap.insert( std::pair<int,int>( ci->structindex, cidx));
			}
		}
		{
			// [1.4] Calculate the level of heading elements for weighting
			std::vector<StructureHeaderReference>::iterator hi = headerar.begin(), he = headerar.end();
			for (; hi != he; ++hi)
			{
				StructIndexContentMapRange crg = structIndexContentMap.equal_range( hi->structindex);
				StructIndexContentMap::const_iterator ci = crg.first, ce = crg.second;
				int minlevel = hi->level;
				for (; ci != ce; ++ci)
				{
					const StructureContentReference& content = contentar[ ci->second];
					if (content.field.cover( hi->field) && minlevel > content.level)
					{
						minlevel = content.level;
					}
				}
				hi->level = minlevel;
			}
		}
		// [2] Calculate the queue for searching complete title path solutions:
		std::set<StructureCoverSearch> queue;
		{
			std::vector<StructureHeaderReference>::iterator
				ei = headerar.begin(), ee = headerar.end();
			for (; ei != ee; ++ei)
			{
				int comsumedPostings = ei->queryPostingRange.second - ei->queryPostingRange.first;
				double weight = (double)(comsumedPostings) / (double)m_postingarsize;

				if (ei->level == 0)
				{
					// ... initial field must be top level and not coverd by any other field
					if (comsumedPostings == m_postingarsize)
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
							queue.insert( StructureCoverSearch( content.field, ei->level, weight, ei->queryPostingRange));
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
				if (hi->level > search.level && search.field.cover( hi->field) && !search.overlapsPostingsUsed( hi->queryPostingRange))
				{
					int comsumedPostings = hi->queryPostingRange.second - hi->queryPostingRange.first;
					double weight = search.weight
							+ (((double)(comsumedPostings) / (double)m_postingarsize)
								* m_levelWeight[ hi->level]);

					if (comsumedPostings + (int)search.postingsUsed.size() == m_postingarsize)
					{
						// ... all query features used
						m_lastResult.push_back( WeightedField( hi->field, weight));
						if (m_maxNofResults && m_maxNofResults == (int)m_lastResult.size()) break;
					}
					else if (hi->completeMatch())
					{
						// ... not all query features used but complete match of the title
						StructureCoverSearch newsearch( strus::IndexRange(), hi->level, weight, hi->queryPostingRange);
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

std::string WeightingFunctionContextTitle::debugCall( const Index& docno)
{
	try
	{
		std::ostringstream out;
		out << string_format( _TXT( "calculate %s"), THIS_METHOD_NAME) << std::endl;

		const std::vector<WeightedField>& res = call( docno);
		std::vector<WeightedField>::const_iterator ri = res.begin(), re = res.end();
		for (; ri != re; ++ri)
		{
			out << string_format( _TXT("result field=[%d,%d], weight=%.5f"), (int)ri->field().start(), (int)ri->field().end(), ri->weight()) << std::endl;
		}
		return out.str();
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error calling weighting function '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
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
		return new WeightingFunctionContextTitle( storage, m_hierarchyWeightFactor, m_maxNofResults, m_errorhnd);
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

