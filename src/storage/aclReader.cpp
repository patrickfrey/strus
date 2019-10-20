/*
 * Copyright (c) 2017 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "aclReader.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "storageClient.hpp"
#include "databaseAdapter.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>

using namespace strus;

AclReader::AclReader( const StorageClient* storage_, ErrorBufferInterface* errorhnd_)
	:m_storage(storage_),m_usermap(),m_docno(0),m_errorhnd(errorhnd_)
{
	DatabaseAdapter_UserName::Cursor unameCursor( m_storage->databaseClient());
	std::string username;
	Index userno;
	if (unameCursor.loadFirst( username, userno))
	{
		do
		{
			m_usermap[ userno] = username;
		}
		while (unameCursor.loadNext( username, userno));
	}
}

void AclReader::skipDoc( const Index& docno_)
{
	m_docno = docno_ ? docno_ : 1;
}

std::vector<std::string> AclReader::getReadAccessList() const
{
	try
	{
		std::vector<std::string> rt;
		if (m_storage->withAcl())
		{
			IndexSetIterator iter( m_storage->getAclIterator( m_docno));
			Index elem = iter.skip( 1);
			for (; elem; elem=iter.skip( elem+1))
			{
				std::map<Index,std::string>::const_iterator ui = m_usermap.find( elem);
				if (ui == m_usermap.end())
				{
					throw strus::runtime_error(_TXT("internal: user %u not defined"), elem);
				}
				rt.push_back( ui->second);
			}
		}
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error reading ACL of document: %s"), *m_errorhnd, std::vector<std::string>());
}


