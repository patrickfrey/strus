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
#include "strus/postingIteratorInterface.hpp"
#include "strus/forwardIteratorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/attributeReaderInterface.hpp"
#include <boost/scoped_ptr.hpp>
#include <iostream>
#include <sstream>

using namespace strus;

StorageDocumentChecker::StorageDocumentChecker(
		Storage* storage_,
		const std::string& docid_)
	:m_storage(storage_)
	,m_docid(docid_)
	,m_docno(storage_->documentNumber( docid_))
{
}

StorageDocumentChecker::~StorageDocumentChecker()
{
}

void StorageDocumentChecker::addTermOccurrence(
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

void StorageDocumentChecker::done()
{
	try
	{
		TermMap::const_iterator ti = m_termMap.begin(), te = m_termMap.end();
		for (; ti != te; ++ti) try
		{
			boost::scoped_ptr<PostingIteratorInterface> pitr(
				m_storage->createTermPostingIterator( ti->first.type, ti->first.value)); 
			boost::scoped_ptr<ForwardIteratorInterface> fitr(
				m_storage->createForwardIterator( ti->first.type));
	
			std::set<Index>::reverse_iterator
				pi = ti->second.poset.rbegin(), pe = ti->second.poset.rend();
	
			if (m_docno != pitr->skipDoc( m_docno))
			{
				throw std::runtime_error( "term not found in inverted index");
			}
			fitr->skipDoc( m_docno);
			Index pos = 0;
	
			for (; pi != pe; ++pi)
			{
				Index ppos = pitr->skipPos( pos);
				if (*pi != ppos)
				{
					std::ostringstream msg;
					msg << *pi << " != " << ppos;
					throw std::runtime_error( std::string( "inverted index position does not match: ") + msg.str());
				}
				Index fpos = fitr->skipPos( pos);
				if (*pi != fpos)
				{
					std::ostringstream msg;
					msg << *pi << " != " << fpos;
					throw std::runtime_error( std::string( "forward index position does not match: ") + msg.str());
				}
				std::string fval = fitr->fetch();
				if (ti->first.value != fval)
				{
					throw std::runtime_error( std::string( "forward index element does not match: '") + ti->first.value + "' != '" + fval + "'");
				}
				pos = *pi + 1;
			}
		}
		catch (const std::runtime_error& err)
		{
			throw std::runtime_error( std::string( "checking term '") + ti->first.type + " '" + ti->first.value + "': " + err.what());
		}

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
				throw std::runtime_error( std::string( "document metadata does not match: '") + mi->second.tostring() + "' != '" + val.tostring() + "'");
			}
		}

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
				throw std::runtime_error( std::string( "document attribute does not match: '") + ai->second + "' != '" + val + "'");
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		throw std::runtime_error( std::string( "error checking document '") + m_docid + "': " + err.what());
	}
}


