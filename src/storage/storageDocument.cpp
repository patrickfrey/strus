/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storageDocument.hpp"
#include "storage.hpp"
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/errorBufferInterface.hpp"
#include <string>
#include <cstring>
#include <limits>
#include <set>

using namespace strus;

StorageDocument::StorageDocument(
		StorageTransaction* transaction_,
		const std::string& docid_,
		strus::Index docno_,
		ErrorBufferInterface* errorhnd_)
	:m_transaction(transaction_)
	,m_docid(docid_)
	,m_docno(docno_)
	,m_terms(),m_invs()
	,m_structBuilder( docid_, docno_, errorhnd_)
	,m_attributes(),m_metadata(),m_userlist()
	,m_maxpos(0)
	,m_nofStructuresIgnored(0)
	,m_errorhnd(errorhnd_)
{}

StorageDocument::~StorageDocument()
{}

TermMapKey StorageDocument::termMapKey( const std::string& type_, const std::string& value_)
{
	Index typeno = m_transaction->getOrCreateTermType( type_);
	Index valueno = m_transaction->getOrCreateTermValue( value_);
	return TermMapKey( typeno, valueno);
}

void StorageDocument::addSearchIndexTerm(
		const std::string& type_,
		const std::string& value_,
		const Index& position_)
{
	try
	{
		if (position_ <= 0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT( "term occurrence position must be >= 1 (term %s '%s')"), type_.c_str(), value_.c_str());
		}
		else
		{
			if (position_ > m_maxpos)
			{
				m_maxpos = position_;
			}
			if (position_ <= (strus::Index)Constants::storage_max_position_info())
			{
				TermMapKey key( termMapKey( type_, value_));
				TermMapValue& ref = m_terms[ key];
				ref.pos.insert( position_);
			}
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding search index term to document %s: %s"), m_docid.c_str(), *m_errorhnd);
}

void StorageDocument::addSearchIndexStructure(
		const std::string& struct_,
		const IndexRange& source_,
		const IndexRange& sink_)
{
	try
	{
		if (source_.start() <= 0 || source_.end() <= 0 || sink_.start() <= 0 || sink_.end() <= 0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT( "structure range positions must be >= 1 (structure '%s')"), struct_.c_str());
		}
		if (source_.end() > m_maxpos)
		{
			m_maxpos = source_.end();
		}
		if (sink_.end() > m_maxpos)
		{
			m_maxpos = source_.end();
		}
		if (source_.end() <= std::numeric_limits<StructBlock::PositionType>::max()
		&&  sink_.end() <= std::numeric_limits<StructBlock::PositionType>::max())
		{
			Index structno = m_transaction->getOrCreateStructType( struct_);
			m_structBuilder.append( structno, source_, sink_);
		}
		else
		{
			m_nofStructuresIgnored += 1;
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding search index structure to document %s: %s"), m_docid.c_str(), *m_errorhnd);
}

void StorageDocument::addForwardIndexTerm(
		const std::string& type_,
		const std::string& value_,
		const Index& position_)
{
	try
	{
		if (position_ <= 0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT( "term occurrence position must be >= 1 (term %s '%s')"), type_.c_str(), value_.c_str());
		}
		else
		{
			if (position_ > m_maxpos)
			{
				m_maxpos = position_;
			}
			if (position_ <= (strus::Index)Constants::storage_max_position_info())
			{
				Index typeno = m_transaction->getOrCreateTermType( type_);
				m_invs[ InvMapKey( typeno, position_)] = value_;
			}
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding forward index structure to document %s: %s"), m_docid.c_str(), *m_errorhnd);
}

void StorageDocument::setMetaData(
		const std::string& name_,
		const NumericVariant& value_)
{
	try
	{
		m_metadata.push_back( DocMetaData( name_, value_));
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding meta data to document %s: %s"), m_docid.c_str(), *m_errorhnd);
}

void StorageDocument::setAttribute(
		const std::string& name_,
		const std::string& value_)
{
	try
	{
		m_attributes.push_back( DocAttribute( name_, value_));
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding attribute to document %s: %s"), m_docid.c_str(), *m_errorhnd);
}

void StorageDocument::setUserAccessRight(
		const std::string& username_)
{
	try
	{
		m_userlist.push_back( m_transaction->getOrCreateUserno( username_));
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error setting user rights of document %s: %s"), m_docid.c_str(), *m_errorhnd);
}

void StorageDocument::done()
{
	try
	{
		if (m_maxpos >= (strus::Index)Constants::storage_max_position_info())
		{
			m_errorhnd->info( _TXT("token positions are out of range (document too big, only %d of %d token positions assigned), %d structures dropped"), (int)(strus::Index)Constants::storage_max_position_info(), (int)m_maxpos, (int)m_nofStructuresIgnored);
		}
		//[1.1] Delete old metadata:
		m_transaction->deleteMetaData( m_docno);
		//[1.2] Delete old attributes:
		m_transaction->deleteAttributes( m_docno);
		//[1.2] Delete old structures:
		m_transaction->deleteStructures( m_docno);
		//[1.3] Delete old index elements (forward index and inverted index):
		m_transaction->deleteIndex( m_docno);
		//[1.4] Delete old user access rights:
		m_transaction->deleteAcl( m_docno);

		//[2.1] Define new metadata:
		std::vector<DocMetaData>::const_iterator wi = m_metadata.begin(), we = m_metadata.end();
		for (; wi != we; ++wi)
		{
			m_transaction->defineMetaData( m_docno, wi->name, wi->value);
		}
		//[2.2] Insert new attributes:
		std::vector<DocAttribute>::const_iterator ai = m_attributes.begin(), ae = m_attributes.end();
		for (; ai != ae; ++ai)
		{
			m_transaction->defineAttribute( m_docno, ai->name, ai->value);
		}
		//[2.3] Insert new index elements (forward index, inverted index and structures):
		TermMap::const_iterator ti = m_terms.begin(), te = m_terms.end();
		for (; ti != te; ++ti)
		{
			//[2.3.1] Insert inverted index
			std::vector<Index> pos( ti->second.pos.begin(), ti->second.pos.end());
			m_transaction->definePosinfoPosting(
					ti->first.first, ti->first.second, m_docno, pos);
		}
		//[2.3.2] Build (compress) and insert structures:
		if (!m_structBuilder.empty())
		{
			m_transaction->defineStructureBlock( m_docno, m_structBuilder.createBlock());
		}
		m_transaction->openForwardIndexDocument( m_docno);
		InvMap::const_iterator ri = m_invs.begin(), re = m_invs.end();
		for (; ri != re; ++ri)
		{
			//[2.3.3] Insert forward index
			m_transaction->defineForwardIndexTerm(
				ri->first.typeno, ri->first.pos, ri->second);
		}
		m_transaction->closeForwardIndexDocument();

		//[2.4] Insert new document access rights:
		std::vector<Index>::const_iterator ui = m_userlist.begin(), ue = m_userlist.end();
		for (; ui != ue; ++ui)
		{
			m_transaction->defineAcl( *ui, m_docno);
		}

		//[3] Clear data:
		m_terms.clear();
		m_invs.clear();
		m_structBuilder.clear();
		m_attributes.clear();
		m_metadata.clear();
		m_userlist.clear();
		m_maxpos = 0;
		m_nofStructuresIgnored = 0;
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error finishing document %s: %s"), m_docid.c_str(), *m_errorhnd);
}


