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
#include "storageDocument.hpp"
#include "storage.hpp"
#include "indexPacker.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <cstring>
#include <set>

using namespace strus;

StorageDocument::StorageDocument(
		StorageTransaction* transaction_,
		const std::string& docid_,
		const Index& docno_,
		bool isNew_)
	:m_transaction(transaction_)
	,m_docid(docid_)
	,m_docno(docno_)
	,m_isNew(isNew_)
{}

StorageDocument::~StorageDocument()
{}

StorageDocument::TermMapKey StorageDocument::termMapKey( const std::string& type_, const std::string& value_)
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
	if (position_ == 0) throw strus::runtime_error( _TXT( "term occurrence position must not be 0"));

	TermMapKey key( termMapKey( type_, value_));
	TermMapValue& ref = m_terms[ key];
	ref.pos.insert( position_);
}

void StorageDocument::addForwardIndexTerm(
		const std::string& type_,
		const std::string& value_,
		const Index& position_)
{
	Index typeno = m_transaction->getOrCreateTermType( type_);
	m_invs[ InvMapKey( typeno, position_)] = value_;
}

void StorageDocument::setMetaData(
		const std::string& name_,
		const ArithmeticVariant& value_)
{
	m_metadata.push_back( DocMetaData( name_, value_));
}

void StorageDocument::setAttribute(
		const std::string& name_,
		const std::string& value_)
{
	m_attributes.push_back( DocAttribute( name_, value_));
}

void StorageDocument::setUserAccessRight(
		const std::string& username_)
{
	bool isNew = false;
	m_userlist.push_back( m_transaction->getOrCreateUserno( username_, isNew));
}

void StorageDocument::done()
{
	if (!m_isNew)
	{
		//[1.1] Delete old metadata:
		m_transaction->deleteMetaData( m_docno);
		//[1.2] Delete old attributes:
		m_transaction->deleteAttributes( m_docno);
		//[1.3] Delete old index elements (forward index and inverted index):
		m_transaction->deleteIndex( m_docno);
		//[1.4] Delete old user access rights:
		m_transaction->deleteAcl( m_docno);
	}
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

	//[2.3] Insert new index elements (forward index and inverted index):
	TermMap::const_iterator ti = m_terms.begin(), te = m_terms.end();
	for (; ti != te; ++ti)
	{
		//[2.3.1] Insert inverted index
		std::vector<Index> pos;
		pos.insert( pos.end(), ti->second.pos.begin(), ti->second.pos.end());
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

	//[2.4] Insert new document access rights:
	std::vector<Index>::const_iterator ui = m_userlist.begin(), ue = m_userlist.end();
	for (; ui != ue; ++ui)
	{
		m_transaction->defineAcl( *ui, m_docno);
	}

	m_terms.clear();
	m_invs.clear();
	m_attributes.clear();
	m_metadata.clear();
	m_userlist.clear();
}





StorageDocumentUpdate::StorageDocumentUpdate(
		StorageTransaction* transaction_,
		const Index& docno_)
	:m_transaction(transaction_)
	,m_docno(docno_)
	,m_doClearUserlist(false)
{}

StorageDocumentUpdate::~StorageDocumentUpdate()
{}

void StorageDocumentUpdate::setMetaData(
		const std::string& name_,
		const ArithmeticVariant& value_)
{
	m_metadata.push_back( DocMetaData( name_, value_));
}

void StorageDocumentUpdate::setAttribute(
		const std::string& name_,
		const std::string& value_)
{
	m_attributes.push_back( DocAttribute( name_, value_));
}

void StorageDocumentUpdate::clearAttribute(
		const std::string& name_)
{
	m_attributes.push_back( DocAttribute( name_, ""));
}

void StorageDocumentUpdate::setUserAccessRight(
		const std::string& username_)
{
	bool isNew = false;
	Index usrno = m_transaction->getOrCreateUserno( username_, isNew);
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
	m_add_userlist.push_back( m_transaction->getOrCreateUserno( username_, isNew));
}

void StorageDocumentUpdate::clearUserAccessRight(
		const std::string& username_)
{
	bool isNew = false;
	Index usrno = m_transaction->getOrCreateUserno( username_, isNew);
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

void StorageDocumentUpdate::clearUserAccessRights()
{
	m_add_userlist.clear();
	m_del_userlist.clear();
	m_doClearUserlist = true;
}

void StorageDocumentUpdate::done()
{
	//[1] Update metadata:
	std::vector<DocMetaData>::const_iterator wi = m_metadata.begin(), we = m_metadata.end();
	for (; wi != we; ++wi)
	{
		m_transaction->defineMetaData( m_docno, wi->name, wi->value);
	}

	//[2] Update attributes:
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

	//[3] Update document access rights:
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

	//[3] Clear data:
	m_attributes.clear();
	m_metadata.clear();
	m_add_userlist.clear();
	m_del_userlist.clear();
	m_doClearUserlist = false;
}

