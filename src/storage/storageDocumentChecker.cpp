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
#include "uintCompaction.hpp"
#include "structBlockDeclaration.hpp"
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
#include "strus/base/string_conv.hpp"
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
	,m_termMap(),m_invTermMap(),m_metaDataMap(),m_attributeMap(),m_structurelist(),m_userlist()
	,m_docid(docid_)
	,m_docno(storage_->documentNumber( docid_))
	,m_maxpos(0)
	,m_nofStructuresIgnored(0)
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
		}
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
		if (source_.start() <= 0 || source_.end() <= 0 || sink_.start() <= 0 || sink_.end() <= 0)
		{
			m_errorhnd->report( ErrorCodeInvalidArgument, _TXT( "structure range positions must be >= 1 (structure '%s')"), struct_.c_str());
		}
		else
		{
			if (source_.end() <= (strus::Index)Constants::storage_max_position_info()
			&&  sink_.end() <= (strus::Index)Constants::storage_max_position_info())
			{
				std::string structnam = strus::string_conv::tolower( struct_);
				m_structurelist.insert( Structure( structnam, source_, sink_));
			}
			else
			{
				m_nofStructuresIgnored += 1;
			}
		}
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
				m_invTermMap[ InvKey( type_, position_)] = value_;
			}
		}
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
	char msgbuf[ 2048];
	va_list ap;
	va_start(ap, format);
	int msglen = std::vsnprintf( msgbuf, sizeof(msgbuf), format, ap);
	logout << _TXT("error checking document") << " '" << docid + "': ";
	if (msglen < 0)
	{
		logout << _TXT("format string error");
	}
	else
	{
		if (msglen >= (int)sizeof(msgbuf))
		{
			char* msgptr = msgbuf;
			msgptr[ msglen-3] = '.';
			msgptr[ msglen-2] = '.';
			msgptr[ msglen-1] = '.';
			msgptr[ msglen] = 0;
		}
		logout << msgbuf;
	}
	logout << std::endl;
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

			strus::local_ptr<PostingIteratorInterface> fitr(
				m_storage->createFrequencyPostingIterator( ti->first.type, ti->first.value)); 
			strus::local_ptr<PostingIteratorInterface> pitr(
				m_storage->createTermPostingIterator( ti->first.type, ti->first.value, 1)); 
			if (!fitr.get())
			{
				logError( logout, m_docid, _TXT("failed to create %s checking search index: %s"), "frequency posting iterator", m_errorhnd->fetchError());
				break;
			}
			if (!pitr.get())
			{
				logError( logout, m_docid, _TXT("failed to create %s checking search index: %s"), "term posting iterator", m_errorhnd->fetchError());
				break;
			}
			if (m_docno != pitr->skipDoc( m_docno))
			{
				logError( logout, m_docid, _TXT("term %s '%s' not found in inverted index"), ti->first.type.c_str(), ti->first.value.c_str());
				continue;
			}
			if (m_docno != fitr->skipDoc( m_docno))
			{
				logError( logout, m_docid, _TXT("term %s '%s' not found in inverted index"), ti->first.type.c_str(), ti->first.value.c_str());
				continue;
			}
			if (m_docno != docnoIterator.skip( m_docno))
			{
				logError( logout, m_docid,
						_TXT("term %s '%s' not found in boolean document index"), ti->first.type.c_str(), ti->first.value.c_str());
			}
			int nofpos = pitr->frequency();
			int ff_p = strus::uintFromCompaction( strus::compactUint( nofpos));
			int ff = fitr->frequency();
			if (ff != ff_p)
			{
				logError( logout, m_docid,
						_TXT("term %s '%s' frequency differs in frequency posting iterator and term posting iterator: frequency ~%d != ~%d occurrencies counted (values compacted)"), ti->first.type.c_str(), ti->first.value.c_str(), ff, ff_p);
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
		StructureList structures;
		strus::local_ptr<ValueIteratorInterface> vitr( m_storage->createStructTypeIterator());
		if (vitr.get())
		{
			structnamear = vitr->fetchValues( m_storage->maxStructTypeNo()+2);
		}
		else
		{
			logError( logout, m_docid, _TXT("failed to fetch structure names: %s"), m_errorhnd->fetchError());
		}
		std::map<strus::Index,std::string> structIdMap;
		{
			std::vector<std::string>::const_iterator si = structnamear.begin(), se = structnamear.end();
			for (; si != se; ++si)
			{
				structIdMap[ m_storage->structTypeNumber( *si)] = *si;
			}
		}
		strus::local_ptr<StructIteratorInterface> stitr( m_storage->createStructIterator());
		if (!stitr.get())
		{
			logError( logout, m_docid, _TXT("failed to create structure iterator: %s"), m_errorhnd->fetchError());
		}
		stitr->skipDoc( m_docno);
		{
			typedef std::pair<strus::Index,int> StructKey;
			typedef std::pair<strus::IndexRange,strus::IndexRange> StructRelation;
			typedef std::vector<StructRelation> StructRelationList;
			typedef std::map<StructKey, StructRelationList> StructMap;
			StructMap structMap;

			int li = 0, le = stitr->levels();
			for (; li != le; ++li)
			{
				IndexRange field = stitr->skipPos( li, 0);
				for (; field.defined(); field = stitr->skipPos( li, field.end()))
				{
					StructIteratorInterface::StructureLinkArray lnka = stitr->links( li);
					int ai = 0, ae = lnka.nofLinks();
					for (; ai != ae; ++ai)
					{
						const StructIteratorInterface::StructureLink& link = lnka[ ai];
						StructKey key( link.structno(), link.index());
						std::pair<StructMap::iterator,bool> ins = structMap.insert( StructMap::value_type( key, StructRelationList()));
						StructRelationList& rlist = ins.first->second;
						if (link.header())
						{
							if (rlist.empty())
							{
								rlist.push_back( StructRelation( field, strus::IndexRange()));
							}
							else
							{
								StructRelationList::iterator ri = rlist.begin(), re = rlist.end();
								for (; ri != re; ++ri)
								{
									if (ri->first.defined()) throw std::runtime_error(_TXT("corrupt index: structure with more than one header element"));
									ri->first = field;
								}
							}
						}
						else
						{
							if (rlist.empty())
							{
								rlist.push_back( StructRelation( strus::IndexRange(), field));
							}
							else if (rlist.back().second.defined())
							{
								rlist.push_back( StructRelation( rlist.back().first, field));
							}
							else
							{
								rlist.back().second = field;
							}
						}
					}
				}
			}
			StructMap::const_iterator si = structMap.begin(), se = structMap.end();
			for (; si != se; ++si)
			{
				std::string structName = structIdMap[ si->first.first];
				StructRelationList::const_iterator ri = si->second.begin(), re = si->second.end();
				for (; ri != re; ++ri)
				{
					structures.insert( Structure( structName, ri->first, ri->second));
				}
			}
		}
		if (structures.size() != m_structurelist.size())
		{
			logError( logout, m_docid, _TXT("number of structures in index (%d) not as expected (%d)"), (int)structures.size(), (int)m_structurelist.size());
		}
		StructureList::const_iterator ai = structures.begin(), ae = structures.end();
		StructureList::const_iterator bi = m_structurelist.begin(), be = m_structurelist.end();
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

void StorageDocumentChecker::joinAdjacentStructureMembers( StructureList& structurelist)
{
	StructureList newlist;
	StructureList::iterator si = structurelist.begin(), se = structurelist.end();
	while (si != se)
	{
		StructureList::iterator sn = si;
		++sn;
		Structure newst( *si);
		for (; sn != se && sn->source == si->source && sn->structname == si->structname && sn->sink.start() == newst.sink.end(); ++sn)
		{
			newst.sink = strus::IndexRange( newst.sink.start(), sn->sink.end());
		}
		newlist.insert( newst);
		si = sn;
	}
	structurelist.swap( newlist);
}

void StorageDocumentChecker::done()
{
	try
	{
		if (m_maxpos >= (strus::Index)Constants::storage_max_position_info())
		{
			m_errorhnd->info( _TXT("token positions are out of range (document too big, only %d of %d token positions assigned), %d structures dropped"), (int)(strus::Index)Constants::storage_max_position_info(), (int)m_maxpos, (int)m_nofStructuresIgnored);
		}
		joinAdjacentStructureMembers( m_structurelist);
		if (m_logfile == "-")
		{
			doCheck( std::cout);
		}
		else if (m_logfile.empty())
		{
			std::ostringstream out;
			doCheck( out);
			std::string res = out.str();
			if (!res.empty())
			{
				char const* eoln = std::strchr( res.c_str(), '\n');
				while (eoln && eoln - res.c_str() < 20)
				{
					res[ eoln - res.c_str()] = ' ';
					eoln = std::strchr( eoln+1, '\n');
				}
				if (eoln)
				{
					res.resize( eoln - res.c_str());
				}
				m_errorhnd->report( ErrorCodeDataCorruption, _TXT("document storage inconsistent: %s"), res.c_str());
			}
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


