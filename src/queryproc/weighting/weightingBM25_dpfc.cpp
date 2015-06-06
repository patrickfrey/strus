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
#include "weightingBM25_dpfc.hpp"
#include "strus/constants.hpp"
#include "private/fixedStructAllocator.hpp"
#include <cmath>
#include <ctime>
#include <set>
#include <limits>

using namespace strus;

WeightingExecutionContextBM25_dpfc::WeightingExecutionContextBM25_dpfc(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		float k1_,
		float b_,
		float avgDocLength_,
		const std::string& attribute_content_doclen_,
		const std::string& attribute_title_doclen_,
		unsigned int proximityMinDist_,
		float title_ff_incr_,
		float sequence_ff_incr_,
		float sentence_ff_incr_,
		float relevant_df_factor_)
	:m_k1(k1_),m_b(b_),m_avgDocLength(avgDocLength_)
	,m_nofCollectionDocuments(storage->globalNofDocumentsInserted())
	,m_weight_featar()
	,m_struct_featar()
	,m_title_itr(0),m_metadata(metadata_)
	,m_metadata_content_doclen(metadata_->elementHandle( attribute_content_doclen_.empty()?Constants::metadata_doclen():attribute_content_doclen_))
	,m_metadata_title_doclen(attribute_title_doclen_.size()?metadata_->elementHandle( attribute_title_doclen_):-1)
	,m_proximityMinDist(proximityMinDist_)
	,m_title_ff_incr(title_ff_incr_)
	,m_sequence_ff_incr(sequence_ff_incr_)
	,m_sentence_ff_incr(sentence_ff_incr_)
	,m_relevant_df_factor(relevant_df_factor_)
{}

void WeightingExecutionContextBM25_dpfc::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		float weight_)
{
	if (utils::caseInsensitiveEquals( name_, "match"))
	{
		float nofMatches = itr_->documentFrequency();
		float idf = 0.0;
		bool relevant = (m_nofCollectionDocuments * m_relevant_df_factor > nofMatches);

		if (m_nofCollectionDocuments > nofMatches * 2)
		{
			idf = logf(
					(m_nofCollectionDocuments - nofMatches + 0.5)
					/ (nofMatches + 0.5));
		}
		if (idf < 0.00001)
		{
			idf = 0.00001;
		}
		m_weight_featar.push_back( Feature( itr_, weight_, idf, relevant));
	}
	else if (utils::caseInsensitiveEquals( name_, "struct"))
	{
		m_struct_featar.push_back( Feature( itr_, weight_, 0.0, false));
	}
	else if (utils::caseInsensitiveEquals( name_, "title"))
	{
		if (m_title_itr) throw strus::runtime_error( _TXT( "duplicate '%s' weighting function feature parameter '%s'"), "BM25_dpfc", name_.c_str());
		m_title_itr = itr_;
	}
	else
	{
		throw strus::runtime_error( _TXT( "unknown '%s' weighting function feature parameter '%s'"), "BM25_dpfc", name_.c_str());
	}
}


struct FeatStruct
{
	Index itridx;
	Index pos;

	FeatStruct()
		:itridx(0),pos(0){}
	FeatStruct( const Index& itridx_, const Index& pos_)
		:itridx(itridx_),pos(pos_){}
	FeatStruct( const FeatStruct& o)
		:itridx(o.itridx),pos(o.pos){}

	bool operator<( const FeatStruct& o) const
	{
		if (pos == o.pos) return itridx < o.itridx;
		return (pos < o.pos);
	}
};

enum {MaxNofFeatures=256};
typedef std::set<
		FeatStruct,
		std::less<FeatStruct>,
		FixedStructAllocator<FeatStruct,MaxNofFeatures> > FeatStructSet;


static void handleSequence( const FeatStructSet& wset, float* accu, float weight, const Index& docno)
{
	FeatStructSet::iterator first = wset.begin();
	FeatStructSet::iterator next = first; ++next;

	for (;next != wset.end() && next->pos == first->pos+1; ++next)
	{
		if (next->itridx == first->itridx+1)
		{
			accu[ first->itridx] += weight;
			accu[ next->itridx] += weight;
		}
		else if (next->itridx+1 == first->itridx)
		{
			accu[ first->itridx] += weight/2;
			accu[ next->itridx] += weight/2;
		}
	}
}

static Index handleSameSentence( std::vector<WeightingExecutionContextBM25_dpfc::Feature>& struct_featar, const FeatStructSet& wset, float* accu, const Index& startPos, float weight, const Index& docno)
{
	typedef WeightingExecutionContextBM25_dpfc::Feature Feature;
	std::vector<Feature>::iterator si = struct_featar.begin(), se = struct_featar.end();
	Index pos;
	Index endOfSentence = std::numeric_limits<Index>::max();
	for (; si != se; ++si)
	{
		pos = si->itr->skipPos( startPos);
		if (pos != 0 && pos < endOfSentence)
		{
			endOfSentence = pos;
		}
	}
	FeatStructSet::iterator first = wset.begin();
	FeatStructSet::iterator next = first; ++next;

	for (;next != wset.end() && next->pos < endOfSentence; ++next)
	{
		if (next->pos >= startPos)
		{
			accu[ first->itridx] += weight;
			accu[ next->itridx] += weight;
		}
	}
	return endOfSentence;
}


float WeightingExecutionContextBM25_dpfc::call( const Index& docno)
{
	float rt = 0.0;
	FeatStructSet wset;
	float accu[ MaxNofFeatures];

	if (m_weight_featar.size() > MaxNofFeatures || m_struct_featar.size() > MaxNofFeatures)
	{
		throw strus::runtime_error( _TXT("to many features passed to weighting function '%s'"), "BM25_dpfc");
	}

	// Initialize accumulator for ff increments
	Index title_start = 1;
	Index title_end = 0;
	if (m_title_itr && docno == m_title_itr->skipDoc( docno))
	{
		title_start = m_title_itr->skipPos(0);
	}
	if (m_metadata_title_doclen >= 0)
	{
		title_end = title_start + (unsigned int)m_metadata->getValue( m_metadata_title_doclen);
	}
	std::vector<Feature>::const_iterator fi = m_weight_featar.begin(), fe = m_weight_featar.end();
	for (Index fidx=0; fi != fe; ++fi,++fidx)
	{
		accu[ fidx] = 0.0;
		if (docno == fi->itr->skipDoc( docno))
		{
			Index firstpos = fi->itr->skipPos(0);
			if (firstpos)
			{
				wset.insert( FeatStruct( fidx, firstpos));
			}
			if (firstpos >= title_start && firstpos < title_end)
			{
				accu[ fidx] += m_title_ff_incr;
			}
		}
	}

	// Calculate ff increments based on proximity criteria:
	while (wset.size() > 1)
	{
		std::vector<Feature>::const_iterator si = m_struct_featar.begin(), se = m_struct_featar.end();
		for (; si != se; ++si)
		{
			si->itr->skipDoc( docno);
		}
		// [0] Small optimization for not checkin elements with a too big distance (m_proximityMinDist) to others
		FeatStructSet::iterator first = wset.begin();
		FeatStructSet::iterator next = first; ++next;
		for (; next != wset.end(); ++next)
		{
			if (m_weight_featar[ next->itridx].relevant) break;
		}
		if (next == wset.end()) break;

		if (first->pos + (Index)m_proximityMinDist < next->pos)
		{
			FeatStruct elem = *wset.begin();
			wset.erase( wset.begin());
			elem.pos = m_weight_featar[ elem.itridx].itr->skipPos( next->pos-m_proximityMinDist);
			if (elem.pos)
			{
				wset.insert( elem);
			}
			continue;
		}
		// [1] Check sequence of subsequent elements in query:
		handleSequence( wset, accu, m_sequence_ff_incr, docno);

		// [2] Check elements in the same sentence:
		if (m_struct_featar.size() && m_sentence_ff_incr > std::numeric_limits<float>::epsilon())
		{
			Index nextSentence = handleSameSentence( m_struct_featar, wset, accu, first->pos, m_sentence_ff_incr, docno);
			if (nextSentence != std::numeric_limits<Index>::max())
			{
				nextSentence = handleSameSentence( m_struct_featar, wset, accu, nextSentence+1, m_sentence_ff_incr/2, docno);
				if (nextSentence != std::numeric_limits<Index>::max())
				{
					nextSentence = handleSameSentence( m_struct_featar, wset, accu, nextSentence+1, m_sentence_ff_incr/3, docno);
				}
			}
		}
		FeatStruct elem = *wset.begin();
		wset.erase( wset.begin());
		elem.pos = m_weight_featar[ elem.itridx].itr->skipPos( elem.pos+1);
		if (elem.pos)
		{
			wset.insert( elem);
		}
	}

	// Calculate BM25 taking proximity based ff increments into account
	fi = m_weight_featar.begin(), fe = m_weight_featar.end();
	for (Index fidx=0; fi != fe; ++fi,++fidx)
	{
		if (docno==fi->itr->skipDoc( docno))
		{
			m_metadata->skipDoc( docno);
		
			float ff = fi->itr->frequency() + accu[ fidx];
			if (ff == 0.0)
			{
			}
			else if (m_b)
			{
				float doclen = m_metadata->getValue( m_metadata_content_doclen);
				float rel_doclen = (doclen+1) / m_avgDocLength;
				rt += fi->weight * fi->idf
					* (ff * (m_k1 + 1.0))
					/ (ff + m_k1 * (1.0 - m_b + m_b * rel_doclen));
			}
			else
			{
				rt += fi->weight * fi->idf
					* (ff * (m_k1 + 1.0))
					/ (ff + m_k1 * 1.0);
			}
		}
	}
	return rt;
}



void WeightingFunctionInstanceBM25_dpfc::addStringParameter( const std::string& name, const std::string& value)
{
	if (utils::caseInsensitiveEquals( name, "doclen"))
	{
		m_attribute_content_doclen = value;
		if (value.empty()) throw strus::runtime_error( _TXT("empty value passed as '%s' weighting function parameter '%s'"), "BM25_dpfc", name.c_str());
	}
	else if (utils::caseInsensitiveEquals( name, "doclen_title"))
	{
		m_attribute_title_doclen = value;
		if (value.empty()) throw strus::runtime_error( _TXT("empty value passed as '%s' weighting function parameter '%s'"), "BM25_dpfc", name.c_str());
	}
	else if (utils::caseInsensitiveEquals( name, "k1")
	||  utils::caseInsensitiveEquals( name, "b")
	||  utils::caseInsensitiveEquals( name, "avgdoclen")
	||  utils::caseInsensitiveEquals( name, "proxmindist")
	||  utils::caseInsensitiveEquals( name, "titleinc")
	||  utils::caseInsensitiveEquals( name, "seqinc")
	||  utils::caseInsensitiveEquals( name, "strinc")
	||  utils::caseInsensitiveEquals( name, "relevant"))
	{
		addNumericParameter( name, arithmeticVariantFromString( value));
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' textual weighting function parameter '%s'"), "BM25_dpfc", name.c_str());
	}
}

void WeightingFunctionInstanceBM25_dpfc::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
{
	if (utils::caseInsensitiveEquals( name, "k1"))
	{
		m_k1 = (float)value;
	}
	else if (utils::caseInsensitiveEquals( name, "b"))
	{
		m_b = (float)value;
	}
	else if (utils::caseInsensitiveEquals( name, "avgdoclen"))
	{
		m_avgdoclen = (float)value;
	}
	else if (utils::caseInsensitiveEquals( name, "proxmindist"))
	{
		m_proximityMinDist = (unsigned int)value;
	}
	else if (utils::caseInsensitiveEquals( name, "titleinc"))
	{
		m_title_ff_incr = (float)value;
	}
	else if (utils::caseInsensitiveEquals( name, "seqinc"))
	{
		m_sequence_ff_incr = (float)value;
	}
	else if (utils::caseInsensitiveEquals( name, "strinc"))
	{
		m_sentence_ff_incr = (float)value;
	}
	else if (utils::caseInsensitiveEquals( name, "relevant"))
	{
		m_relevant_df_factor = (float)value;
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' numeric weighting function parameter '%s'"), "BM25_dpfc", name.c_str());
	}
}

