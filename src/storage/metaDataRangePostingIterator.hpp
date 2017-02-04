/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of a posting iterator based on a range defined in the meta data (to define data fields without storage block access)
/// \file "metaDataRangePostingIterator.hpp"
#ifndef _STRUS_STORAGE_METADATA_RANGE_POSTING_ITERATOR_HPP_INCLUDED
#define _STRUS_STORAGE_METADATA_RANGE_POSTING_ITERATOR_HPP_INCLUDED
#include "strus/postingIteratorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/index.hpp"
#include "strus/reference.hpp"
#include "private/internationalization.hpp"

namespace strus
{

/// \class MetaDataRangePostingIterator
/// \brief Implementation of a posting iterator based on a range defined in the meta data (to define data fields without storage block access)
class MetaDataRangePostingIterator
	:public PostingIteratorInterface
{
public:
	MetaDataRangePostingIterator( MetaDataReaderInterface* metareader_, Index nofDocuments_, const std::string& name_from, const std::string& name_to, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_)
		,m_metareader(metareader_)
		,m_docno(0)
		,m_handle_lo(name_from.empty()?-1:metareader_->elementHandle(name_from))
		,m_handle_hi(name_to.empty()?-1:metareader_->elementHandle(name_to))
		,m_pos_lo(0)
		,m_pos_hi(0)
		,m_posno(0)
		,m_nofDocuments(nofDocuments_)
	{
		m_featureid.append( name_from);
		m_featureid.push_back( ':');
		m_featureid.append( name_to);
		m_featureid.push_back( ':');
		m_featureid.push_back( 'M');

		if (m_handle_lo < 0 && !name_from.empty()) throw strus::runtime_error(_TXT("failed to define posting iterator from meta data (lower bound): %s"), m_errorhnd->fetchError());
		if (m_handle_hi < 0 && !name_to.empty()) throw strus::runtime_error(_TXT("failed to define posting iterator from meta data (upper bound): %s"), m_errorhnd->fetchError());
	}

	virtual ~MetaDataRangePostingIterator()
	{}

	Index skipDocImpl( const Index& docno)
	{
		if (docno == m_docno && docno) return m_docno;
		m_docno = docno ? (docno-1) : 0;
		m_posno = 0;
		do
		{
			++m_docno;
			if (m_docno >= m_nofDocuments) return m_docno = 0;

			m_metareader->skipDoc( m_docno);
			m_pos_lo = m_handle_lo < 0 ? 1 : (int)m_metareader->getValue( m_handle_lo);
			m_pos_hi = m_handle_hi < 0 ? std::numeric_limits<Index>::max() : (int)m_metareader->getValue( m_handle_hi);
		} while (m_pos_lo == 0 || m_pos_hi == 0 || m_pos_lo >= m_pos_hi);
		return m_docno;
	}

	virtual Index skipDoc( const Index& docno)
	{
		return skipDocImpl( docno);
	}

	virtual Index skipDocCandidate( const Index& docno)
	{
		return skipDocImpl( docno);
	}

	virtual Index skipPos( const Index& firstpos)
	{
		if (firstpos < m_pos_lo) return m_posno = m_pos_lo;
		if (firstpos >= m_pos_hi) return m_posno = 0;
		return m_posno = firstpos;
	}

	virtual const char* featureid() const
	{
		return m_featureid.c_str();
	}

	virtual Index documentFrequency() const
	{
		return m_nofDocuments;
	}

	virtual unsigned int frequency()
	{
		return m_pos_hi - m_pos_lo;
	}

	virtual Index docno() const
	{
		return m_docno;
	}

	virtual Index posno() const
	{
		return m_posno;
	}

	virtual Index length() const
	{
		return m_posno?1:0;
	}

private:
	ErrorBufferInterface* m_errorhnd;
	Reference<MetaDataReaderInterface> m_metareader;
	Index m_docno;
	Index m_handle_lo;
	Index m_handle_hi;
	Index m_pos_lo;
	Index m_pos_hi;
	Index m_posno;
	Index m_nofDocuments;
	std::string m_featureid;
};

}//namespace
#endif


