/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
#include "strus/valueIteratorInterface.hpp"
#include "strus/structIteratorInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/base/snprintf.h"
#include "strus/base/local_ptr.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>

using namespace strus;

StorageDocumentChecker::StorageDocumentChecker(
		const StorageClient* storage_,
		const std::string& docid_,
		const std::string& logfile_,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_)
	,m_docid(docid_)
	,m_docno(storage_->documentNumber( docid_))
	,m_logfile(logfile_)
	,m_errorhnd(errorhnd_)
{
	if (!m_docno)
	{
		throw strus::runtime_error( _TXT( "docid not defined '%s'"), m_docid.c_str());
	}
}

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

void StorageDocumentChecker::addSearchIndexStructure(
		const std::string& struct_,
		const IndexRange& source_,
		const IndexRange& sink_)
{
	try
	{
		m_structurelist.push_back( Structure( struct_, source_, sink_));
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
		const NumericVariant& value_)
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
	{
		//[1] Check term index:
		TermMap::const_iterator ti = m_termMap.begin(), te = m_termMap.end();
		for (; ti != te; ++ti)
		{
			Index typeno = m_storage->getTermType( ti->first.type);
			Index termno = m_storage->getTermValue( ti->first.value);
	
			if (!typeno) throw strus::runtime_error( _TXT( "unknown term type '%s'"), ti->first.type.c_str());
			if (!termno) throw strus::runtime_error( _TXT( "unknown term value '%s'"), ti->first.value.c_str());
	
			IndexSetIterator docnoIterator( m_storage->databaseClient(), DatabaseKey::DocListBlockPrefix, BlockKey( typeno, termno), false);
	
			strus::local_ptr<PostingIteratorInterface> pitr(
				m_storage->createTermPostingIterator( ti->first.type, ti->first.value, 1)); 
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
	}{
		//[2] Check structures:
		std::vector<std::string> structnamear;
		std::vector<Structure> structures;
		strus::local_ptr<ValueIteratorInterface> vitr( m_storage->createStructTypeIterator());
		if (vitr.get())
		{
			structnamear = vitr->fetchValues( m_storage->maxStructTypeNo()+2);
		}
		else
		{
			logError( logout, m_docid, _TXT("failed to fetch structure names: %s"), m_errorhnd->fetchError());
		}
		std::vector<std::string>::const_iterator si = structnamear.begin(), se = structnamear.end();
		for (; si != se; ++si)
		{
			strus::local_ptr<StructIteratorInterface> stitr( m_storage->createStructIterator( *si));
			if (!stitr.get())
			{
				logError( logout, m_docid, _TXT("failed to create structure iterator: %s"), m_errorhnd->fetchError());
				break;
			}
			Index docno = stitr->skipDoc( 0);
			for (; docno; docno = stitr->skipDoc( docno+1))
			{
				IndexRange source = stitr->skipPosSource( 0);
				for (; source.defined(); source = stitr->skipPosSource( source.end()))
				{
					IndexRange sink = stitr->skipPosSink( 0);
					for (; sink.defined(); sink = stitr->skipPosSource( sink.end()))
					{
						structures.push_back( Structure( *si, source, sink));
					}
				}
			}
		}
		std::sort( structures.begin(), structures.end());
		if (structures.size() != m_structurelist.size())
		{
			logError( logout, m_docid, _TXT("number of structures in index (%d) not as expected (%d)"), (int)structures.size(), (int)m_structurelist.size());
		}
		std::vector<Structure>::const_iterator ai = structures.begin(), ae = structures.end();
		std::vector<Structure>::const_iterator bi = m_structurelist.begin(), be = m_structurelist.end();
		while (ai != ae && bi != be)
		{
			if (*ai < *bi)
			{
				logError( logout, m_docid, _TXT("unexpected structure in index: %s [%d,%d]-> [%d,%d]"), ai->structname.c_str(), (int)ai->source.start(), (int)ai->source.end(), (int)ai->sink.start(), (int)ai->sink.end());
				++ai;
			}
			else if (*bi < *ai)
			{
				logError( logout, m_docid, _TXT("missing structure in index: %s [%d,%d]-> [%d,%d]"), bi->structname.c_str(), (int)bi->source.start(), (int)bi->source.end(), (int)bi->sink.start(), (int)bi->sink.end());
				++bi;
			}
			else
			{
				ai++;
				bi++;
			}
		}
		for (; ai != ae; ++ai)
		{
			logError( logout, m_docid, _TXT("unexpected structure in index: %s [%d,%d]-> [%d,%d]"), ai->structname.c_str(), (int)ai->source.start(), (int)ai->source.end(), (int)ai->sink.start(), (int)ai->sink.end());
		}
		for (; bi != be; ++bi)
		{
			logError( logout, m_docid, _TXT("missing structure in index: %s [%d,%d]-> [%d,%d]"), bi->structname.c_str(), (int)bi->source.start(), (int)bi->source.end(), (int)bi->sink.start(), (int)bi->sink.end());
		}
	}{
		//[3] Check forward index:
		InvTermMap::const_iterator vi = m_invTermMap.begin(), ve = m_invTermMap.end();
		for (; vi != ve; ++vi)
		{
			strus::local_ptr<ForwardIteratorInterface> fitr(
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
	}{
		//[4] Check meta data:
		strus::local_ptr<MetaDataReaderInterface> metadata(
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
				Index hnd = metadata->elementHandle( mi->first);
				if (hnd < 0)
				{
					logError( logout, m_docid,
						_TXT( "unknown document meta data element '%s'"), mi->first.c_str());
					continue;
				}
				metadata->skipDoc( m_docno);
		
				NumericVariant val = metadata->getValue( hnd);
				if (val != mi->second)
				{
					logError( logout, m_docid,
						_TXT( "document meta data does not match for '%s': '%s' != '%s'"), mi->first.c_str(), mi->second.tostring().c_str(), val.tostring().c_str());
				}
			}
		}
	}{
		//[5] Check attributes:
		strus::local_ptr<AttributeReaderInterface> attributes(
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
				Index hnd = attributes->elementHandle( ai->first);
				if (hnd == 0)
				{
					logError( logout, m_docid,
						_TXT( "unknown document attribute '%s'"), ai->first.c_str());
					continue;
				}
				attributes->skipDoc( m_docno);
		
				std::string val = attributes->getValue( hnd);
				if (val != ai->second)
				{
					logError( logout, m_docid,
						_TXT( "document attribute does not match for '%s': '%s' != '%s'"), ai->first.c_str(), ai->second.c_str(), val.c_str());
				}
			}
		}
	}{
		//[6] Check access rights:
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
		std::sort( m_structurelist.begin(), m_structurelist.end());
		if (m_logfile == "-")
		{
			doCheck( std::cout);
		}
		else if (m_logfile.empty())
		{
			doCheck( std::cerr);
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


