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

using namespace strus;
#define THIS_METHOD_NAME const_cast<char*>("title")
#undef STRUS_LOWLEVEL_DEBUG

WeightingFunctionContextTitle::WeightingFunctionContextTitle(
		const StorageClientInterface* storage_,
		double hierarchyWeightFactor_,
		GlobalCounter nofCollectionDocuments_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_structitr(storage_->createStructIterator()),m_postingarsize(0)
	,m_hierarchyWeightFactor(hierarchyWeightFactor_)
	,m_nofCollectionDocuments(nofCollectionDocuments_)
	,m_lastResult(),m_errorhnd(errorhnd_)
{
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
			if (m_postingarsize > MaxNofArguments) throw strus::runtime_error( _TXT("number of weighting features (%d) out of range"), m_postingarsize);

			double df = termstats.documentFrequency()>=0?termstats.documentFrequency():(GlobalCounter)itr->documentFrequency();
			double idf = strus::Math::log10( (m_nofCollectionDocuments - df + 0.5) / (df + 0.5));

			if (idf < 0.00001)
			{
				idf = 0.00001;
			}
			m_postingar[ m_postingarsize++] = Posting( itr, idf, weight);
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

struct StructureHeaderEval
{
	int contentlevel;
	int structindex;
	strus::Index structno;
	strus::IndexRange headerfield;
	strus::bitset<WeightingFunctionContextTitle::MaxTitleSize> headerposet;
	strus::bitset<WeightingFunctionContextTitle::MaxNofArguments> featureset;
	int parentlink;

	StructureHeaderEval( int contentlevel_, int structindex_, strus::Index structno_, const strus::IndexRange& headerfield_)
		:contentlevel(contentlevel_),structindex(structindex_),structno(structno_)
		,headerfield(headerfield_),headerposet(),featureset()
		,parentlink(-1)
	{}
	StructureHeaderEval( const StructureHeaderEval& o)
		:contentlevel(o.contentlevel),structindex(o.structindex),structno(o.structno)
		,headerfield(o.headerfield),headerposet(o.headerposet),featureset(o.featureset)
		,parentlink(o.parentlink)
	{}
	void setFeature( int featidx, strus::Index posno)
	{
		headerposet.set( posno - headerfield.start(), true);
		featureset.set( featidx, true);
	}
	void setParentLink( int evalidx)
	{
		parentlink = evalidx;
	}
};

const std::vector<WeightedField>& WeightingFunctionContextTitle::call( const Index& docno)
{
	try
	{
		std::vector<StructureHeaderEval> evalar;
		evalar.reserve( EvalAllocSize);

		m_lastResult.resize( 0);
		m_structitr->skipDoc(docno);

		// [1] Fill header elements:
		int pi = 0, pe = m_postingarsize;
		for (; pi != pe; ++pi)
		{
			int li=0, le = m_structitr->levels();
			for (; li != le; ++li)
			{
				strus::Index posno = m_postingar[ pi].iterator->skipPos( 0);
				for (; posno; posno = m_postingar[ pi].iterator->skipPos( posno+1))
				{
					strus::IndexRange field = m_structitr->skipPos( li, posno);
					if (!field.defined()) break;
					if (field.len() < MaxTitleSize && field.contain( posno))
					{
						StructureLinkArray lnkar = m_structitr->links( li);
						int si=0, se = lnkar.nofLinks();
						for (; si != se; ++si)
						{
							const StructureLink& lnk = lnkar[ si];
							if (lnk.header())
							{
								std::vector<StructureHeaderEval>::iterator
									ei = evalar.begin(), ee = evalar.end();
								for (; ei != ee; ++ei)
								{
									if (ei->structindex == lnk.index())
									{
										if (ei->headerfield != field) throw std::runtime_error(_TXT("corrupt index: structure with more than one header"));
										ei->setFeature( pi, posno);
										break;
									}
								}
								if (ei == ee)
								{
									evalar.push_back( StructureHeaderEval( 
										li, lnk.index(), lnk.structno(), field));
									evalar.back().setFeature( pi, posno);
								}
							}
						}
						posno = field.end()-1;//... Count feature only once
					}
				}
			}
		}
		// [2] Calculate content dependencies of structure headers:
		int li=0, le = m_structitr->levels();
		for (; li != le; ++li)
		{
			std::vector<StructureHeaderEval>::iterator
				ei = evalar.begin(), ee = evalar.end();
			for (; ei != ee; ++ei)
			{
				strus::IndexRange field = m_structitr->skipPos( li, ei->headerfield.start());
				if (!field.defined()) break;
				if (field.cover( ei->headerfield))
				{
					StructureLinkArray lnkar = m_structitr->links( li);
					int si=0, se = lnkar.nofLinks();
					for (; si != se; ++si)
					{
						const StructureLink& lnk = lnkar[ si];
						if (!lnk.header())
						{
							std::vector<StructureHeaderEval>::iterator
								oi = evalar.begin(), oe = evalar.end();
							for (int oidx=0; oi != oe; ++oi,++oidx)
							{
								if (oi->structindex == lnk.index())
								{
									ei->setParentLink( oidx);
								}
							}
						}
					}
				}
			}
		}
		// [3] Find complete path of structure header inclusions:
		std::vector<StructureHeaderEval>::iterator
			ei = evalar.begin(), ee = evalar.end();
		for (int eidx=0; ei != ee; ++ei,++eidx)
		{
			int ti = eidx;
			strus::bitset<WeightingFunctionContextTitle::MaxNofArguments> featureset;
			double weight = 0.0;
			int level = -1;

			for (; ti >= 0; ti = evalar[ ti].parentlink)
			{
				const StructureHeaderEval& stu = evalar[ ti];
				level = stu.contentlevel;
				if (ti != eidx && (int)stu.headerposet.size() != (int)stu.headerfield.len()) break;
				weight *= m_hierarchyWeightFactor;
				weight += (double)stu.headerposet.size() / (double)m_postingarsize;
				featureset.join( stu.featureset);
			}
			if (ti == -1 && level == 0 && (int)featureset.size() == m_postingarsize)
			{
				m_lastResult.push_back( WeightedField( ei->headerfield, weight));
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
		GlobalCounter nofCollectionDocuments = stats.nofDocumentsInserted()>=0?stats.nofDocumentsInserted():(GlobalCounter)storage->nofDocumentsInserted();
		return new WeightingFunctionContextTitle( storage, m_hierarchyWeightFactor, nofCollectionDocuments, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' weighting function: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView WeightingFunctionInstanceTitle::view() const
{
	try
	{
		StructView rt;
		rt( "hf", m_hierarchyWeightFactor);
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
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating weighting function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

