/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerMatchesBase.hpp"
#include "postingIteratorLink.hpp"
#include "private/ranker.hpp"
#include "strus/numericVariant.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include "weightedValue.hpp"
#include "textNormalizer.hpp"
#include <limits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

SummarizerFunctionContextMatchesBase::SummarizerFunctionContextMatchesBase(
		const StorageClientInterface* storage_,
		const MatchesBaseParameter& parameter_,
		const char* name_,
		ErrorBufferInterface* errorhnd_)
	:m_parameter(parameter_),m_name(name_)
	,m_storage(storage_)
	,m_tag_forwardindex()
	,m_text_forwardindex()
	,m_features()
	,m_cur_featureidx(-1)
	,m_cur_pos(0)
	,m_end_pos(1)
	,m_cur_value()
	,m_cur_weight(0.0)
	,m_errorhnd(errorhnd_)
{
	if (!m_parameter.tagtype.empty())
	{
		m_tag_forwardindex.reset( m_storage->createForwardIterator( m_parameter.tagtype));
		if (!m_tag_forwardindex.get())
		{
			throw std::runtime_error( _TXT("error creating forward index iterator for tags"));
		}
	}
	if (!m_parameter.texttype.empty())
	{
		m_text_forwardindex.reset( m_storage->createForwardIterator( m_parameter.texttype));
		if (!m_text_forwardindex.get())
		{
			throw std::runtime_error( _TXT("error creating forward index iterator for text"));
		}
	}
}

void SummarizerFunctionContextMatchesBase::setVariableValue( const std::string&, double)
{
	m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables known for function '%s'"), m_name);
}


enum VariableType {
	VariableNone=0,
	VariableTag=1,
	VariableText=2,
	VariablePosition=3
};
enum {
	VariableTypeShift=2,
	VariableTypeMask=3
};

static void defineVariable( strus::NamedFormatString& fmt, const std::string& varname, int varidx)
{
	fmt.assign( strus::string_format( "pos:%s", varname.c_str()), (varidx << VariableTypeShift) + VariablePosition);
	fmt.assign( strus::string_format( "tag:%s", varname.c_str()), (varidx << VariableTypeShift) + VariableTag);
	fmt.assign( strus::string_format( "text:%s", varname.c_str()), (varidx << VariableTypeShift) + VariableText);
	fmt.assign( varname, (varidx << VariableTypeShift) + VariableText);
}

void SummarizerFunctionContextMatchesBase::addSummarizationFeature(
		const std::string& name_,
		PostingIteratorInterface* itr,
		const std::vector<SummarizationVariable>& variables,
		double weight)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match"))
		{
			strus::NamedFormatString fmt( m_parameter.fmt, ":", m_errorhnd);
			std::set<std::string> variableset;

			std::vector<const PostingIteratorInterface*> varitr;
			std::vector<SummarizationVariable>::const_iterator vi = variables.begin(), ve = variables.end();
			for (; vi != ve; ++vi)
			{
				if (false == variableset.insert( strus::string_conv::tolower( vi->name())).second)
				{
					throw strus::runtime_error( _TXT( "more than one variable assigned with the name '%s'"), vi->name().c_str());
					return;
				}
				else
				{
					defineVariable( fmt, vi->name(), varitr.size()+1);
					varitr.push_back( vi->itr());
				}
			}
			fmt.assign( "pos", VariablePosition);
			fmt.assign( "tag", VariableTag);
			fmt.assign( "text", VariableText);
			m_features.push_back( SummarizationFeature( itr, varitr, fmt, weight));
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarization feature '%s'"), m_name, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding feature to '%s' summarizer: %s"), m_name, *m_errorhnd);
}

void SummarizerFunctionContextMatchesBase::start( const strus::WeightedDocument& doc)
{
	if (m_tag_forwardindex.get()) m_tag_forwardindex->skipDoc( doc.docno());
	if (m_text_forwardindex.get()) m_text_forwardindex->skipDoc( doc.docno());
	std::vector<SummarizationFeature>::iterator fi = m_features.begin(), fe = m_features.end();
	for (; fi != fe; ++fi)
	{
		fi->valid = (doc.docno() == fi->itr->skipDoc( doc.docno()));
	}
	m_cur_featureidx = 0;
	m_cur_weight = 0.0;
	m_cur_pos = doc.field().defined() ? (doc.field().start() -1) : 0;
	m_end_pos = doc.field().defined() ? (doc.field().end()) : std::numeric_limits<strus::Index>::max();
	m_cur_value.clear();
}

bool SummarizerFunctionContextMatchesBase::next()
{
	if (m_cur_featureidx < 0) return false;
	for (; m_cur_featureidx < (int)m_features.size(); ++m_cur_featureidx)
	{
		const SummarizationFeature& feat = m_features[ m_cur_featureidx];
		if (!feat.valid) continue;

		m_cur_weight = feat.weight;
		m_cur_pos = feat.itr->skipPos( m_cur_pos+1);
		if (m_cur_pos && m_cur_pos < m_end_pos)
		{
			m_cur_value.clear();
			NamedFormatString::const_iterator ni = feat.fmt.begin(), ne = feat.fmt.end();
			for (; ni != ne; ++ni)
			{
				m_cur_value.append( ni->prefix());
				if (ni->idx())
				{
					char postr[ 64];
					int varidx = ni->idx() >> VariableTypeShift;
					VariableType vartype = (VariableType)(ni->idx() & VariableTypeMask);
					strus::Index pos = (varidx == 0)
							? m_cur_pos
							: feat.varitr[ varidx-1]->posno();
					switch (vartype)
					{
						case VariableNone:
							break;
						case VariablePosition:
							std::snprintf( postr, sizeof(postr), "%d", (int)pos);
							m_cur_value.append( postr);
							break;
						case VariableTag:
							if (m_tag_forwardindex.get())
							{
								if (pos == m_tag_forwardindex->skipPos( pos))
								{
									m_cur_value.append(
										strus::stripForwardIndexText(
											m_tag_forwardindex->fetch(),
											m_parameter.stripCharacters));
								}
							}
							else
							{
								throw std::runtime_error(_TXT("tag referenced but no forward index iterator defined for tag matches"));
							}
							break;
						case VariableText:
							if (m_text_forwardindex.get())
							{
								if (pos == m_text_forwardindex->skipPos( pos))
								{
									m_cur_value.append(
										strus::stripForwardIndexText(
											m_text_forwardindex->fetch(),
											m_parameter.stripCharacters));
								}
							}
							else
							{
								throw std::runtime_error(_TXT("text referenced but no forward index iterator defined for text matches"));
							}
							break;
					}
				}
			}
			return true;
		}
	}
	return false;
}

static NumericVariant parameterValue( const std::string& name_, const std::string& value)
{
	NumericVariant rt;
	if (!rt.initFromString(value.c_str())) throw strus::runtime_error(_TXT("numeric value expected as parameter '%s' (%s)"), name_.c_str(), value.c_str());
	return rt;
}

void SummarizerFunctionInstanceMatchesBase::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "match"))
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as string"), name_.c_str(), m_name);
		}
		else if (strus::caseInsensitiveEquals( name_, "text"))
		{
			m_parameter.texttype = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "tag"))
		{
			m_parameter.tagtype = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "fmt"))
		{
			m_parameter.fmt = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "strip"))
		{
			m_parameter.stripCharacters = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "results"))
		{
			addNumericParameter( name_, parameterValue( name_, value));
		}
		else
		{
			throw strus::runtime_error( _TXT("unknown '%s' summarization function parameter '%s'"), m_name, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), m_name, *m_errorhnd);
}

void SummarizerFunctionInstanceMatchesBase::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	if (strus::caseInsensitiveEquals( name_, "match"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("parameter '%s' for summarizer '%s' expected to be defined as feature and not as numeric value"), name_.c_str(), m_name);
	}
	else if (strus::caseInsensitiveEquals( name_, "text")
		|| strus::caseInsensitiveEquals( name_, "tag")
		|| strus::caseInsensitiveEquals( name_, "fmt")
		|| strus::caseInsensitiveEquals( name_, "strip"))
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("no numeric value expected for parameter '%s' in summarization function '%s'"), name_.c_str(), m_name);
	}
	else if (strus::caseInsensitiveEquals( name_, "results"))
	{
		m_parameter.maxNofElements = value.toint();
	}
	else
	{
		m_errorhnd->report( ErrorCodeInvalidArgument, _TXT("unknown '%s' summarization function parameter '%s'"), m_name, name_.c_str());
	}
}

StructView SummarizerFunctionInstanceMatchesBase::view() const
{
	try
	{
		StructView rt;
		rt( "text", m_parameter.texttype);
		rt( "tag", m_parameter.tagtype);
		rt( "fmt", m_parameter.fmt);
		rt( "strip", m_parameter.stripCharacters);
		rt( "results", m_parameter.maxNofElements);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), m_name, *m_errorhnd, std::string());
}


StructView SummarizerFunctionMatchesBase::view() const
{
	try
	{
		typedef FunctionDescription P;
		const char* description = 0;
		if (0 == std::strcmp( m_name, "accumatch"))
		{
			description = _TXT("Accumulate all contents of matching expressions printed with a format string.");
		}
		else if (0 == std::strcmp( m_name, "listmatch"))
		{
			description = _TXT("List all contents of matching expressions printed with a format string.");
		}
		else
		{
			throw std::runtime_error(_TXT("logic error: bad name"));
		}
		FunctionDescription rt( name(), description);
		rt( P::Feature, "match", _TXT( "defines the query features for collecting the matches"), "");
		rt( P::String, "text", _TXT( "the forward index feature type for the content to extract"), "");
		rt( P::String, "tag", _TXT( "the forward index feature type for the tag info to extract"), "");
		rt( P::String, "fmt", _TXT( "format string for building the summary element values referencing variables in curly brackets"), "");
		rt( P::Numeric, "results", _TXT( "the maximum number of the best weighted elements  to return (default 30)"), "1:");
		return std::move(rt);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), m_name, *m_errorhnd, FunctionDescription());
}

