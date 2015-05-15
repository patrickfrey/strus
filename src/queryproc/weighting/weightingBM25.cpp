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
#include "weightingBM25.hpp"
#include "strus/constants.hpp"
#include <cmath>
#include <ctime>

using namespace strus;

WeightingExecutionContextBM25::WeightingExecutionContextBM25(
		const StorageClientInterface* storage,
		MetaDataReaderInterface* metadata_,
		float k1_,
		float b_,
		float avgDocLength_,
		const std::string& attribute_doclen_)
	:m_k1(k1_),m_b(b_),m_avgDocLength(avgDocLength_)
	,m_nofCollectionDocuments(storage->globalNofDocumentsInserted())
	,m_featar(),m_metadata(metadata_)
	,m_metadata_doclen(metadata_->elementHandle( attribute_doclen_.empty()?Constants::metadata_doclen():attribute_doclen_))
{}

void WeightingExecutionContextBM25::addWeightingFeature(
		const std::string& name_,
		PostingIteratorInterface* itr_,
		float weight_)
{
	if (utils::caseInsensitiveEquals( name_, "match"))
	{
		float nofMatches = itr_->documentFrequency();
		float idf = 0.0;
	
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
		m_featar.push_back( Feature( itr_, weight_, idf));
	}
	else
	{
		throw strus::runtime_error( _TXT( "unknown '%s' weighting function feature parameter '%s'"), "BM25", name_.c_str());
	}
}


float WeightingExecutionContextBM25::call( const Index& docno)
{
	float rt = 0.0;
	std::vector<Feature>::const_iterator fi = m_featar.begin(), fe = m_featar.end();
	for ( ;fi != fe; ++fi)
	{
		if (docno==fi->itr->skipDoc( docno))
		{
			m_metadata->skipDoc( docno);
		
			float ff = fi->itr->frequency();
			if (ff == 0.0)
			{
			}
			else if (m_b)
			{
				float doclen = m_metadata->getValue( m_metadata_doclen);
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




void WeightingFunctionInstanceBM25::addStringParameter( const std::string& name, const std::string& value)
{
	if (utils::caseInsensitiveEquals( name, "doclen"))
	{
		m_attribute_doclen = value;
		if (value.empty()) throw strus::runtime_error( _TXT("empty value passed as '%s' weighting function parameter '%s'"), "BM25", name.c_str());
	}
	if (utils::caseInsensitiveEquals( name, "k1")
	||  utils::caseInsensitiveEquals( name, "b")
	||  utils::caseInsensitiveEquals( name, "avgdoclen"))
	{
		addNumericParameter( name, arithmeticVariantFromString( value));
	}
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' weighting function parameter '%s'"), "BM25", name.c_str());
	}
}

void WeightingFunctionInstanceBM25::addNumericParameter( const std::string& name, const ArithmeticVariant& value)
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
	else
	{
		throw strus::runtime_error( _TXT("unknown '%s' weighting function parameter '%s'"), "BM25", name.c_str());
	}
}


bool WeightingFunctionInstanceBM25::isFeatureParameter( const std::string& name) const
{
	return utils::caseInsensitiveEquals( name, "match");
}


