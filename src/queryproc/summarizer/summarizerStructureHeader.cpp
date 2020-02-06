/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "summarizerStructureHeader.hpp"
#include "postingIteratorLink.hpp"
#include "weightedValue.hpp"
#include "structureSearch.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/postingJoinOperatorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/constants.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/configParser.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "private/functionDescription.hpp"
#include "viewUtils.hpp"
#include <limits>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace strus;

#define THIS_METHOD_NAME const_cast<char*>("structheader")

SummarizerFunctionContextStructureHeader::SummarizerFunctionContextStructureHeader(
			const StorageClientInterface* storage_,
			const SummarizerFunctionParameterStructureHeader& parameter_,
			ErrorBufferInterface* errorhnd_)
	:m_parameter(parameter_)
	,m_storage(storage_)
	,m_structiter( storage_->createStructIterator())
	,m_structno(parameter_.structName.empty() ? 0:storage_->structTypeNumber(parameter_.structName))
	,m_headerar()
	,m_textCollector( storage_, parameter_.textType, errorhnd_)
	,m_errorhnd(errorhnd_)
				 
{
	if (!m_structiter.get())
	{
		throw strus::runtime_error(_TXT("failed to create structure iterator for '%s': %s"), THIS_METHOD_NAME, m_errorhnd->fetchError());
	}
}

void SummarizerFunctionContextStructureHeader::addSummarizationFeature(
			const std::string& name,
			PostingIteratorInterface* itr,
			const std::vector<SummarizationVariable>& variables,
			double weight,
			const TermStatistics&)
{
	throw strus::runtime_error(_TXT("added feature '%s' but no feature parameters expected for '%s'"), name.c_str(), THIS_METHOD_NAME);
}

void SummarizerFunctionContextStructureHeader::setVariableValue( const std::string& name, double value)
{
	throw strus::runtime_error(_TXT("added variable '%s' but no variables expected for '%s'"), name.c_str(), THIS_METHOD_NAME);
}

std::vector<SummaryElement> SummarizerFunctionContextStructureHeader::getSummary( const strus::WeightedDocument& doc)
{
	try
	{
		std::vector<SummaryElement> rt;
		m_headerar.clear();
		strus::collectHeaderFields( m_headerar, m_structiter.get(), m_structno, doc.docno(), doc.field());
		m_textCollector.skipDoc( doc.docno());
		std::vector<HeaderField>::const_iterator hi = m_headerar.begin(), he = m_headerar.end();
		for (int hidx=0; hi != he; ++hi,++hidx)
		{
			
			rt.push_back( SummaryElement( "", m_textCollector.fetch( hi->field()), 1.0, hi->level()));
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summary: %s"), THIS_METHOD_NAME, *m_errorhnd, std::vector<SummaryElement>());
}

void SummarizerFunctionInstanceStructureHeader::addStringParameter( const std::string& name_, const std::string& value)
{
	try
	{
		if (strus::caseInsensitiveEquals( name_, "text"))
		{
			if (!m_parameter.textType.empty()) throw strus::runtime_error(_TXT("parameter '%s' defined twice"), name_.c_str());
			m_parameter.textType = value;
		}
		else if (strus::caseInsensitiveEquals( name_, "struct"))
		{
			if (!m_parameter.structName.empty()) throw strus::runtime_error(_TXT("parameter '%s' defined twice"), name_.c_str());
			m_parameter.structName = value;
		}
		else
		{
			m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown '%s' summarizer function parameter '%s'"), THIS_METHOD_NAME, name_.c_str());
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding string parameter to '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd);
}

void SummarizerFunctionInstanceStructureHeader::addNumericParameter( const std::string& name_, const NumericVariant& value)
{
	m_errorhnd->report( ErrorCodeUnknownIdentifier, _TXT("unknown numeric parameter '%s' for summarizer function '%s'"), name_.c_str(), THIS_METHOD_NAME);
}

SummarizerFunctionContextInterface* SummarizerFunctionInstanceStructureHeader::createFunctionContext(
			const StorageClientInterface* storage,
			const GlobalStatistics&) const
{
	if (m_parameter.textType.empty())
	{
		m_errorhnd->report( ErrorCodeIncompleteInitialization, _TXT( "no elements defined to collect in '%s'"), THIS_METHOD_NAME);
	}
	try
	{
		return new SummarizerFunctionContextStructureHeader( storage, m_parameter, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating context of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView SummarizerFunctionInstanceStructureHeader::view() const
{
	try
	{
		StructView rt;
		rt( "text", m_parameter.textType);
		rt( "struct", m_parameter.structName);
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error fetching '%s' summarizer introspection view: %s"), THIS_METHOD_NAME, *m_errorhnd, std::string());
}


SummarizerFunctionInstanceInterface* SummarizerFunctionStructureHeader::createInstance(
			const QueryProcessorInterface* processor) const
{
	try
	{
		return new SummarizerFunctionInstanceStructureHeader( m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating instance of '%s' summarizer: %s"), THIS_METHOD_NAME, *m_errorhnd, 0);
}

StructView SummarizerFunctionStructureHeader::view() const
{
	try
	{
		typedef FunctionDescription P;
		FunctionDescription rt( name(), _TXT("Get the titles of the structures with contennts covering the document field."));
		rt( P::String, "text", _TXT( "feature type in forward index where to collect the summary from"), "");
		rt( P::String, "structno", _TXT( "name of structure type to select the structures for the title summaries (optional, use all structures if not defined)"), "");
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating summarizer function description for '%s': %s"), THIS_METHOD_NAME, *m_errorhnd, FunctionDescription());
}

const char* SummarizerFunctionInstanceStructureHeader::name() const
{
	return THIS_METHOD_NAME;
}
const char* SummarizerFunctionStructureHeader::name() const
{
	return THIS_METHOD_NAME;
}


