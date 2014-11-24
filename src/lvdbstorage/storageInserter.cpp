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
#include "storageInserter.hpp"
#include "storage.hpp"
#include "databaseKey.hpp"
#include "indexPacker.hpp"
#include <string>
#include <cstring>
#include <set>
#include <boost/scoped_ptr.hpp>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG

StorageInserter::StorageInserter( Storage* storage_, const std::string& docid_)
	:m_storage(storage_),m_docid(docid_)
{}

StorageInserter::~StorageInserter()
{
	//... nothing done here. The document id and term value or type strings 
	//	created might remain inserted, even after a rollback.
}

StorageInserter::TermMapKey StorageInserter::termMapKey( const std::string& type_, const std::string& value_)
{
	Index typeno = m_storage->getOrCreateTermType( type_);
	Index valueno = m_storage->getOrCreateTermValue( value_);
	return TermMapKey( typeno, valueno);
}

void StorageInserter::addTermOccurrence(
		const std::string& type_,
		const std::string& value_,
		const Index& position_,
		float weight_)
{
	if (position_ == 0) throw std::runtime_error( "term occurrence position must not be 0");

	TermMapKey key( termMapKey( type_, value_));
	TermMapValue& ref = m_terms[ key];
	ref.pos.insert( position_);
	ref.weight += weight_;
	m_invs[ InvMapKey( key.first, position_)] = value_;
}

void StorageInserter::setMetaData(
		char name_,
		float value_)
{
}

void StorageInserter::setMetaData(
		const std::string& name_,
		float value_)
{
	m_metadata_float.push_back( DocMetaData<float>( name_, value_));
}

void StorageInserter::setMetaData(
		const std::string& name_,
		int value_)
{
	m_metadata_int.push_back( DocMetaData<int>( name_, value_));
}

void StorageInserter::setMetaData(
		const std::string& name_,
		unsigned int value_)
{
	m_metadata_uint.push_back( DocMetaData<unsigned int>( name_, value_));
}

void StorageInserter::setAttribute(
		const std::string& name_,
		const std::string& value_)
{
	m_attributes.push_back( DocAttribute( name_, value_));
}

void StorageInserter::done()
{
	bool documentIsNew = false;
	Index docno = m_storage->getOrCreateDocno( m_docid, documentIsNew);

	leveldb::Iterator* vi = m_storage->newIterator();
	boost::scoped_ptr<leveldb::Iterator> viref(vi);

	//[1] Delete old and define new metadata
	m_storage->deleteMetaData( docno);
	{
		std::vector<DocMetaData<float> >::const_iterator wi = m_metadata_float.begin(), we = m_metadata_float.end();
		for (; wi != we; ++wi)
		{
			m_storage->defineMetaData( docno, wi->name, wi->value);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "PUT METADATA [" << docno << ":" << wi->name << "]" << "= [" << wi->value << "]" << std::endl;
#endif
		}
	}
	{
		std::vector<DocMetaData<int> >::const_iterator wi = m_metadata_int.begin(), we = m_metadata_int.end();
		for (; wi != we; ++wi)
		{
			m_storage->defineMetaData( docno, wi->name, wi->value);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "PUT METADATA [" << docno << ":" << wi->name << "]" << "= [" << wi->value << "]" << std::endl;
		}
#endif
	}
	{
		std::vector<DocMetaData<unsigned int> >::const_iterator wi = m_metadata_uint.begin(), we = m_metadata_uint.end();
		for (; wi != we; ++wi)
		{
			m_storage->defineMetaData( docno, wi->name, wi->value);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "PUT METADATA [" << docno << ":" << wi->name << "]" << "= [" << wi->value << "]" << std::endl;
		}
#endif
	}
	//[2] Delete old and insert new attributes
	m_storage->deleteAttributes( docno);

	std::vector<DocAttribute>::const_iterator ai = m_attributes.begin(), ae = m_attributes.end();
	for (; ai != ae; ++ai)
	{
		Index attribno = m_storage->getAttribute( ai->name);
		DatabaseKey docattribkey( (char)DatabaseKey::DocAttributePrefix, docno, attribno);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "PUT ATTRIBUTE [" << docattribkey << "]" << "= [" << wi->value << "]" << std::endl;
#endif
		leveldb::Slice keyslice( docattribkey.ptr(), docattribkey.size());
		m_storage->writeKeyValue( keyslice, ai->value);
	}

	//[3] Delete old and insert new index elements (forward index and inverted index):
	m_storage->deleteIndex( docno);

	TermMap::const_iterator ti = m_terms.begin(), te = m_terms.end();
	for (; ti != te; ++ti)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "PUT TERMS [" << ti->first.first << " " << ti->first.second << " " << docno << "]" << std::endl;
#endif
		std::vector<Index> pos;
		pos.insert( pos.end(), ti->second.pos.begin(), ti->second.pos.end());
		m_storage->definePosinfoPosting(
				ti->first.first, ti->first.second, docno, pos);
		m_storage->defineDocnoPosting(
				ti->first.first, ti->first.second,
				docno, ti->second.pos.size(), ti->second.weight);
		m_storage->incrementDf( ti->first.first, ti->first.second);
	}

	InvMap::const_iterator ri = m_invs.begin(), re = m_invs.end();
	for (; ri != re; ++ri)
	{
		DatabaseKey invkey( (char)DatabaseKey::ForwardIndexPrefix, docno);
		invkey.addElem( ri->first.typeno);	// [typeno]
		invkey.addElem( ri->first.pos);		// [position]

		leveldb::Slice invkeyslice( invkey.ptr(), invkey.size());

#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "PUT INV [" << invkey << "]" << "= [" << ri->second.value << "]" << std::endl;
#endif
		m_storage->writeKeyValue( invkeyslice, ri->second);
	}
	if (documentIsNew)
	{
		m_storage->incrementNofDocumentsInserted();
	}

	// [5] Do submit the write to the database:
	m_storage->checkFlush();
	m_storage->releaseInserter();
}

