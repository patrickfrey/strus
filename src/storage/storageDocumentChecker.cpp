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
#include "storageDocumentChecker.hpp"
#include "storageClient.hpp"
#include "storageTransaction.hpp"
#include "storageDocument.hpp"
#include "storage.hpp"
#include "indexSetIterator.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/attributeReaderInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/private/snprintf.h"
#include <iostream>
#include <sstream>
#include <fstream>

using namespace strus;

StorageDocumentChecker::StorageDocumentChecker(
		const StorageClient* storage_,
		const DatabaseClientInterface* database_,
		const std::string& docid_,
		const std::string& logfile_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_database(database_)
	,m_docid(docid_)
	,m_docno(storage_->documentNumber( docid_))
	,m_logfile(logfile_)
	,m_errorhnd(errorhnd_)
{}

StorageDocumentChecker::~StorageDocumentChecker()
{
}

void StorageDocumentChecker::addSearchIndexTerm(
		const std::string& type_,
		const std::string& value_,
		const Index& position_)
{
	try
	{
		TermMap::iterator ti = m_termMap.find( Term( type_, value_));
		TermAttributes* attributes = 0;
		if (ti == m_termMap.end())
		{
			attributes = &m_termMap[ Term( type_, value_)];
		}
		else
		{
			attributes = &ti->second;
		}
		attributes->poset.insert( position_);
	}
	CATCH_ERROR_MAP( _TXT("error adding search index term: %s"), *m_errorhnd);
}

void StorageDocumentChecker::addForwardIndexTerm(
		const std::string& type_,
		const std::string& value_,
		const Index& position_)
{
	try
	{
		m_invTermMap[ InvKey( type_, position_)] = value_;
	}
	CATCH_ERROR_MAP( _TXT("error adding forward index term: %s"), *m_errorhnd);
}

void StorageDocumentChecker::setMetaData(
		const std::string& name_,
		const ArithmeticVariant& value_)
{
	try
	{
		m_metaDataMap[ name_] = value_;
	}
	CATCH_ERROR_MAP( _TXT("error setting meta data: %s"), *m_errorhnd);
}


void StorageDocumentChecker::setAttribute(
		const std::string& name_,
		const std::string& value_)
{
	try
	{
		m_attributeMap[ name_] = value_;
	}
	CATCH_ERROR_MAP( _TXT("error in transaction commit: %s"), *m_errorhnd);
}

static void logError(
		std::ostream& logout, const std::string& docid,
		const char* format, ...)
{
	char msgbuf[ 1024];
	va_list ap;
	va_start(ap, format);
	strus_vsnprintf( msgbuf, sizeof(msgbuf), format, ap);

	logout << "error checking document '" << docid + "': ";
	logout << msgbuf << std::endl;

	va_end(ap);
}

void StorageDocumentChecker::doCheck( std::ostream& logout)
{
	//[1] Check term index:
	TermMap::const_iterator ti = m_termMap.begin(), te = m_termMap.end();
	for (; ti != te; ++ti)
	{
		Index typeno = m_storage->getTermType( ti->first.type);
		Index termno = m_storage->getTermValue( ti->first.value);

		if (!typeno) throw strus::runtime_error( _TXT( "unknown term type '%s'"), ti->first.type.c_str());
		if (!termno) throw strus::runtime_error( _TXT( "unknown term value '%s'"), ti->first.value.c_str());

		IndexSetIterator docnoIterator( m_database, DatabaseKey::DocListBlockPrefix, BlockKey( typeno, termno), false);

		std::auto_ptr<PostingIteratorInterface> pitr(
			m_storage->createTermPostingIterator( ti->first.type, ti->first.value)); 
		if (!pitr.get())
		{
			logError( logout, m_docid, _TXT("memory allocation error checking search index"));
			break;
		}
		if (m_docno != pitr->skipDoc( m_docno))
		{
			logError( logout, m_docid, _TXT("term %s '%s' not found in inverted index"), ti->first.type.c_str(), ti->first.value.c_str());
			continue;
		}
		if (m_docno != docnoIterator.skip( m_docno))
		{
			logError( logout, m_docid,
					_TXT("term %s '%s' not found in boolean document index"), ti->first.type.c_str(), ti->first.value.c_str());
		}
		Index pos = 0;

		std::set<Index>::const_iterator
			pi = ti->second.poset.begin(), pe = ti->second.poset.end();
		for (pi=ti->second.poset.begin(); pi != pe; ++pi)
		{
			Index ppos = pitr->skipPos( pos);
			if (*pi != ppos)
			{
				logError( logout, m_docid,
					_TXT("term %s '%s' inverted index position does not match: %u != %u"), ti->first.type.c_str(), ti->first.value.c_str(), *pi, ppos);
				break;
			}
			pos = *pi + 1;
		}
	}

	//[2] Check forward index:
	InvTermMap::const_iterator vi = m_invTermMap.begin(), ve = m_invTermMap.end();
	for (; vi != ve; ++vi)
	{
		std::auto_ptr<ForwardIteratorInterface> fitr(
			m_storage->createForwardIterator( vi->first.type));
		if (!fitr.get())
		{
			logError( logout, m_docid, _TXT("memory allocation error checking forward index"));
			break;
		}
		fitr->skipDoc( m_docno);
		Index fpos = fitr->skipPos( vi->first.pos);
		if (vi->first.pos != fpos)
		{
			logError( logout, m_docid,
				_TXT( "forward index position for type %s does not match: %u != %u"), vi->first.type.c_str(), vi->first.pos, fpos);
			break;
		}
		std::string fval = fitr->fetch();
		if (vi->second != fval)
		{
			logError( logout, m_docid,
				_TXT( "forward index element for type %s at position %u does not match: '%s' != '%s'"), vi->first.type.c_str(), vi->first.pos, fval.c_str(), vi->second.c_str());
		}
	}

	//[3] Check meta data:
	std::auto_ptr<MetaDataReaderInterface> metadata(
		m_storage->createMetaDataReader());
	if (!metadata.get())
	{
		logError( logout, m_docid, _TXT("memory allocation error creating metadata reader"));
	}
	else
	{
		MetaDataMap::const_iterator mi = m_metaDataMap.begin(), me = m_metaDataMap.end();
		for (; mi != me; ++mi)
		{
			Index hnd = metadata->elementHandle( mi->first.c_str());
			metadata->skipDoc( m_docno);
	
			ArithmeticVariant val = metadata->getValue( hnd);
			if (val != mi->second)
			{
				logError( logout, m_docid,
					_TXT( "document meta data does not match: '%s' != '%s'"), mi->second.tostring().c_str(), val.tostring().c_str());
			}
		}
	}
	//[4] Check attributes:
	std::auto_ptr<AttributeReaderInterface> attributes(
		m_storage->createAttributeReader());
	if (!attributes.get())
	{
		logError( logout, m_docid, _TXT("memory allocation error creating attribute reader"));
	}
	else
	{
		AttributeMap::const_iterator ai = m_attributeMap.begin(), ae = m_attributeMap.end();
		for (; ai != ae; ++ai)
		{
			Index hnd = attributes->elementHandle( ai->first.c_str());
			attributes->skipDoc( m_docno);
	
			std::string val = attributes->getValue( hnd);
			if (val != ai->second)
			{
				logError( logout, m_docid,
					_TXT( "document attribute does not match: '%s' != '%s'"), ai->second.c_str(), val.c_str());
			}
		}
	}
	//[5] Check access rights:
	std::vector<std::string>::const_iterator ui = m_userlist.begin(), ue = m_userlist.end();
	IndexSetIterator aclitr = m_storage->getAclIterator( m_docno);
	for (; ui != ue; ++ui)
	{
		Index userno = m_storage->getUserno( *ui);
		if (!userno)
		{
			logError( logout, m_docid, _TXT("document user rights do not match (undefined username)"));
		}
		IndexSetIterator invaclitr = m_storage->getUserAclIterator( userno);
		if (m_docno != invaclitr.skip( m_docno))
		{
			logError( logout, m_docid, _TXT("document user rights do not match (document not found in inverted ACL)"));
		}
		if (userno != aclitr.skip( userno))
		{
			logError( logout, m_docid, _TXT("document user rights do not match (user not found in ACL)"));
		}
	}
}

void StorageDocumentChecker::setUserAccessRight( const std::string& username)
{
	try
	{
		m_userlist.push_back( username);
	}
	CATCH_ERROR_MAP( _TXT("error setting user access right: %s"), *m_errorhnd);
}

void StorageDocumentChecker::done()
{
	try
	{
		if (m_logfile == "-")
		{
			doCheck( std::cout);
		}
		else
		{
			std::ofstream logfileout;
			logfileout.open( m_logfile.c_str(), std::ios_base::app);
			doCheck( logfileout);
			logfileout.close();
		}
	}
	CATCH_ERROR_MAP( _TXT("error in transaction commit: %s"), *m_errorhnd);
}


