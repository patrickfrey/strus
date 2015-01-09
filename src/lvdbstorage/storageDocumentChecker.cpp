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
#include "storage.hpp"
#include "storageTransaction.hpp"
#include "storageDocument.hpp"
#include "storage.hpp"
#include "indexSetIterator.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/attributeReaderInterface.hpp"
#include <boost/scoped_ptr.hpp>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace strus;

StorageDocumentChecker::StorageDocumentChecker(
		const Storage* storage_,
		const std::string& docid_,
		const std::string& logfile_)
	:m_storage(storage_)
	,m_docid(docid_)
	,m_docno(storage_->documentNumber( docid_))
	,m_logfile(logfile_)
{
}

StorageDocumentChecker::~StorageDocumentChecker()
{
}

void StorageDocumentChecker::addSearchIndexTerm(
		const std::string& type_,
		const std::string& value_,
		const Index& position_,
		float weight_)
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
	attributes->weight += weight_;
}

void StorageDocumentChecker::addForwardIndexTerm(
		const std::string& type_,
		const std::string& value_,
		const Index& position_)
{
	m_invTermMap[ InvKey( type_, position_)] = value_;
}

void StorageDocumentChecker::setMetaData(
		const std::string& name_,
		const ArithmeticVariant& value_)
{
	m_metaDataMap[ name_] = value_;
}


void StorageDocumentChecker::setAttribute(
		const std::string& name_,
		const std::string& value_)
{
	m_attributeMap[ name_] = value_;
}

static void logError(
		std::ostream& logout, const std::string& docid,
		const std::string& type, const std::string& term,
		const std::string& msg)
{
	logout << "error checking document '" << docid + "', ";
	logout << "term '" << type << "' value '" << term << "': ";
	logout << msg << std::endl;
}

static void logError(
		std::ostream& logout, const std::string& docid,
		const std::string& msg)
{
	logout << "error checking document '" << docid + "': ";
	logout << msg << std::endl;
}

void StorageDocumentChecker::doCheck( std::ostream& logout)
{
	//[1] Check term index:
	TermMap::const_iterator ti = m_termMap.begin(), te = m_termMap.end();
	for (; ti != te; ++ti)
	{
		boost::scoped_ptr<PostingIteratorInterface> pitr(
			m_storage->createTermPostingIterator( ti->first.type, ti->first.value)); 

		if (m_docno != pitr->skipDoc( m_docno))
		{
			logError( logout, m_docid, ti->first.type, ti->first.value,
				"term not found in inverted index");
			continue;
		}
		Index pos = 0;

		std::set<Index>::const_iterator
			pi = ti->second.poset.begin(), pe = ti->second.poset.end();
		for (pi=ti->second.poset.begin(); pi != pe; ++pi)
		{
			Index ppos = pitr->skipPos( pos);
			if (*pi != ppos)
			{
				std::ostringstream msg;
				msg << *pi << " != " << ppos;

				logError( logout, m_docid,
					ti->first.type, ti->first.value,
					std::string( "term inverted index position does not match: ") + msg.str());
				break;
			}
			pos = *pi + 1;
		}
	}

	//[2] Check forward index:
	InvTermMap::const_iterator vi = m_invTermMap.begin(), ve = m_invTermMap.end();
	for (; vi != ve; ++vi)
	{
		boost::scoped_ptr<ForwardIteratorInterface> fitr(
			m_storage->createForwardIterator( vi->first.type));

		fitr->skipDoc( m_docno);
		Index fpos = fitr->skipPos( vi->first.pos);
		if (vi->first.pos != fpos)
		{
			std::ostringstream msg;
			msg << vi->first.pos << " != " << fpos;

			logError( logout, m_docid,
				std::string( "forward index position for type ") + vi->first.type + " does not match: " + msg.str());
			break;
		}
		std::string fval = fitr->fetch();
		if (vi->second != fval)
		{
			std::ostringstream msg;
			msg << vi->first.pos;
			logError( logout, m_docid,
				std::string( "forward index element for type ") + vi->first.type + " at position " + msg.str() + " does not match: '" + fval + "' != '" + vi->second + "'");
		}
	}

	//[3] Check meta data:
	boost::scoped_ptr<MetaDataReaderInterface> metadata(
		m_storage->createMetaDataReader());

	MetaDataMap::const_iterator mi = m_metaDataMap.begin(), me = m_metaDataMap.end();
	for (; mi != me; ++mi)
	{
		Index hnd = metadata->elementHandle( mi->first);
		metadata->skipDoc( m_docno);

		ArithmeticVariant val = metadata->getValue( hnd);
		if (val != mi->second)
		{
			logError( logout, m_docid,
				std::string( "document metadata does not match: '") + mi->second.tostring() + "' != '" + val.tostring() + "'");
		}
	}

	//[4] Check attributes:
	boost::scoped_ptr<AttributeReaderInterface> attributes(
		m_storage->createAttributeReader());

	AttributeMap::const_iterator ai = m_attributeMap.begin(), ae = m_attributeMap.end();
	for (; ai != ae; ++ai)
	{
		Index hnd = attributes->elementHandle( ai->first);
		attributes->skipDoc( m_docno);

		std::string val = attributes->getValue( hnd);
		if (val != ai->second)
		{
			logError( logout, m_docid,
				std::string( "document attribute does not match: '") + ai->second + "' != '" + val + "'");
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
			logError( logout, m_docid, "document user rights do not match (undefined username)");
		}
		IndexSetIterator invaclitr = m_storage->getUserAclIterator( userno);
		if (m_docno != invaclitr.skip( m_docno))
		{
			logError( logout, m_docid, "document user rights do not match (document not found in inverted ACL)");
		}
		if (userno != aclitr.skip( userno))
		{
			logError( logout, m_docid, "document user rights do not match (user not found in ACL)");
		}
	}
}

void StorageDocumentChecker::setUserAccessRights( const std::string& username)
{
	m_userlist.push_back( username);
}

void StorageDocumentChecker::done()
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


