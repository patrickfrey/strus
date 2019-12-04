/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "storageDocumentUpdate.hpp"
#include "storage.hpp"
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/errorBufferInterface.hpp"
#include <string>
#include <cstring>
#include <set>

using namespace strus;

StorageDocumentUpdate::StorageDocumentUpdate(
		StorageTransaction* transaction_,
		const Index& docno_,
		ErrorBufferInterface* errorhnd_)
	:m_transaction(transaction_)
	,m_docno(docno_)
	,m_doClearUserlist(false)
	,m_errorhnd(errorhnd_)
{}

StorageDocumentUpdate::~StorageDocumentUpdate()
{}

TermMapKey StorageDocumentUpdate::termMapKey( const std::string& type_, const std::string& value_)
{
	Index typeno = m_transaction->getOrCreateTermType( type_);
	Index valueno = m_transaction->getOrCreateTermValue( value_);
	return TermMapKey( typeno, valueno);
}

void StorageDocumentUpdate::addSearchIndexTerm(
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
			TermMapKey key( termMapKey( type_, value_));
			TermMapValue& ref = m_terms[ key];
			ref.pos.insert( position_);
			m_delete_search_typenolist.insert( key.first);
		}
	}
	CATCH_ERROR_MAP( _TXT("error adding search index term to document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::addSearchIndexStructure(
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
		else
		{
			Index structno = m_transaction->getOrCreateStructType( struct_);
			m_structures.push_back( DocStructure( structno, source_, sink_));
			m_delete_search_structnolist.insert( structno);
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error adding search index structure '%s' to document: %s"), struct_.c_str(), *m_errorhnd);
}

void StorageDocumentUpdate::addForwardIndexTerm(
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
			Index typeno = m_transaction->getOrCreateTermType( type_);
			m_invs[ InvMapKey( typeno, position_)] = value_;
			m_delete_forward_typenolist.insert( typeno);
		}
	}
	CATCH_ERROR_MAP( _TXT("error adding forward index term to document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::clearSearchIndexTerm(
		const std::string& type_)
{
	try
	{
		Index typeno = m_transaction->getOrCreateTermType( type_);
		m_delete_search_typenolist.insert( typeno);
	}
	CATCH_ERROR_MAP( _TXT("error removing occurrencies of search index term type from document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::clearSearchIndexStructure(
		const std::string& struct_)
{
	try
	{
		Index structno = m_transaction->getOrCreateStructType( struct_);
		m_delete_search_structnolist.insert( structno);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error removing structure '%s' from search index: %s"), struct_.c_str(), *m_errorhnd);
}

void StorageDocumentUpdate::clearForwardIndexTerm(
		const std::string& type_)
{
	try
	{
		Index typeno = m_transaction->getOrCreateTermType( type_);
		m_delete_forward_typenolist.insert( typeno);
	}
	CATCH_ERROR_MAP( _TXT("error removing occurrencies of forward index term type from document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::setMetaData(
		const std::string& name_,
		const NumericVariant& value_)
{
	try
	{
		m_metadata.push_back( DocMetaData( name_, value_));
	}
	CATCH_ERROR_MAP( _TXT("error adding meta data to document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::setAttribute(
		const std::string& name_,
		const std::string& value_)
{
	try
	{
		m_attributes.push_back( DocAttribute( name_, value_));
	}
	CATCH_ERROR_MAP( _TXT("error adding attribute to document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::clearAttribute(
		const std::string& name_)
{
	try
	{
		m_attributes.push_back( DocAttribute( name_, ""));
	}
	CATCH_ERROR_MAP( _TXT("error clear attribute of document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::setUserAccessRight(
		const std::string& username_)
{
	try
	{
		Index usrno = m_transaction->getOrCreateUserno( username_);
		std::vector<Index>::iterator ui = m_del_userlist.begin(), ue = m_del_userlist.end();
		while (ui != ue)
		{
			if (*ui == usrno)
			{
				m_del_userlist.erase( ui);
			}
			else
			{
				++ui;
			}
		}
		m_add_userlist.push_back( m_transaction->getOrCreateUserno( username_));
	}
	CATCH_ERROR_MAP( _TXT("error set user access rights of document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::clearUserAccessRight(
		const std::string& username_)
{
	try
	{
		Index usrno = m_transaction->getOrCreateUserno( username_);
		std::vector<Index>::iterator ui = m_add_userlist.begin(), ue = m_add_userlist.end();
		while (ui != ue)
		{
			if (*ui == usrno)
			{
				m_add_userlist.erase( ui);
			}
			else
			{
				++ui;
			}
		}
		m_del_userlist.push_back( usrno);
	}
	CATCH_ERROR_MAP( _TXT("error clear user access right to document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::clearUserAccessRights()
{
	try
	{
		m_add_userlist.clear();
		m_del_userlist.clear();
		m_doClearUserlist = true;
	}
	CATCH_ERROR_MAP( _TXT("error clear all user access rights of document: %s"), *m_errorhnd);
}

void StorageDocumentUpdate::done()
{
	try
	{
		//[1] Delete old index elements (forward index and inverted index):
		{
			std::set<Index>::const_iterator si = m_delete_search_typenolist.begin(), se = m_delete_search_typenolist.end();
			for (; si != se; ++si)
			{
				m_transaction->deleteDocSearchIndexType( m_docno, *si);
			}
		}{
			std::set<Index>::const_iterator fi = m_delete_forward_typenolist.begin(), fe = m_delete_forward_typenolist.end();
			for (; fi != fe; ++fi)
			{
				m_transaction->deleteDocForwardIndexType( m_docno, *fi);
			}
		}{
			std::set<Index>::const_iterator si = m_delete_search_structnolist.begin(), se = m_delete_search_structnolist.end();
			for (; si != se; ++si)
			{
				m_transaction->deleteStructure( *si, m_docno);
			}
		}{
			//[2.1] Update metadata:
			std::vector<DocMetaData>::const_iterator wi = m_metadata.begin(), we = m_metadata.end();
			for (; wi != we; ++wi)
			{
				m_transaction->defineMetaData( m_docno, wi->name, wi->value);
			}
		}{
			//[2.2] Update attributes:
			std::vector<DocAttribute>::const_iterator ai = m_attributes.begin(), ae = m_attributes.end();
			for (; ai != ae; ++ai)
			{
				if (ai->value.empty())
				{
					m_transaction->deleteAttribute( m_docno, ai->name);
				}
				else
				{
					m_transaction->defineAttribute( m_docno, ai->name, ai->value);
				}
			}
		}{
			//[2.3] Insert new index elements (forward index and inverted index):
			TermMap::const_iterator ti = m_terms.begin(), te = m_terms.end();
			for (; ti != te; ++ti)
			{
				//[2.3.1] Insert inverted index
				std::vector<Index> pos( ti->second.pos.begin(), ti->second.pos.end());
				m_transaction->definePosinfoPosting(
						ti->first.first, ti->first.second, m_docno, pos);
			}
			m_transaction->openForwardIndexDocument( m_docno);
			InvMap::const_iterator ri = m_invs.begin(), re = m_invs.end();
			for (; ri != re; ++ri)
			{
				//[2.3.2] Insert forward index
				m_transaction->defineForwardIndexTerm(
					ri->first.typeno, ri->first.pos, ri->second);
			}
			m_transaction->closeForwardIndexDocument();

			std::vector<DocStructure>::const_iterator si = m_structures.begin(), se = m_structures.end();
			for (; si != se; ++si)
			{
				m_transaction->defineStructure( si->structno, m_docno, si->source, si->sink);
			}
		}{
			//[2.4] Update document access rights:
			if (m_doClearUserlist)
			{
				m_transaction->deleteAcl( m_docno);
			}
			std::vector<Index>::const_iterator ui = m_add_userlist.begin(), ue = m_add_userlist.end();
			for (; ui != ue; ++ui)
			{
				m_transaction->defineAcl( *ui, m_docno);
			}
			ui = m_del_userlist.begin(), ue = m_del_userlist.end();
			for (; ui != ue; ++ui)
			{
				m_transaction->deleteAcl( *ui, m_docno);
			}
		}
		//[3] Clear data:
		m_terms.clear();
		m_invs.clear();
		m_delete_search_typenolist.clear();
		m_delete_forward_typenolist.clear();
		m_delete_search_structnolist.clear();
		m_structures.clear();
		m_attributes.clear();
		m_metadata.clear();
		m_add_userlist.clear();
		m_del_userlist.clear();
		m_doClearUserlist = false;
	}
	CATCH_ERROR_MAP( _TXT("error closing document update in transaction: %s"), *m_errorhnd);
}

