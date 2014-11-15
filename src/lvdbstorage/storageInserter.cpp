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
	Index typeno = m_storage->keyGetOrCreate( DatabaseKey::TermTypePrefix, type_);
	Index valueno = m_storage->keyGetOrCreate( DatabaseKey::TermValuePrefix, value_);
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
	m_metadata.push_back( DocMetaData( name_, value_));
}

void StorageInserter::setAttribute(
		char name_,
		const std::string& value_)
{
	m_attributes.push_back( DocAttribute( name_, value_));
}

void StorageInserter::done()
{
	Index docno = m_storage->keyGetOrCreate( DatabaseKey::DocIdPrefix, m_docid);
	bool documentFound = false;

	leveldb::Iterator* vi = m_storage->newIterator();
	boost::scoped_ptr<leveldb::Iterator> viref(vi);

	// [1] Define new metadata
	std::vector<DocMetaData>::const_iterator wi = m_metadata.begin(), we = m_metadata.end();
	for (; wi != we; ++wi)
	{
		m_storage->defineMetaData( docno, wi->name, wi->value);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "PUT METADATA [" << docno << ":" << wi->name << "]" << "= [" << wi->value << "]" << std::endl;
#endif
	}

	// [1.3] Delete old attributes
	DatabaseKey docattribkey( (char)DatabaseKey::DocAttributePrefix, docno);
	std::size_t docattribkeysize = docattribkey.size();

	for (vi->Seek( leveldb::Slice( docattribkey.ptr(), docattribkeysize));
		vi->Valid(); vi->Next())
	{
		if (docattribkeysize > vi->key().size() || 0!=std::memcmp( vi->key().data(), (char*)docattribkey.ptr(), docattribkeysize))
		{
			//... end of document reached
			break;
		}
		documentFound = true;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "DELETE ATTRIBUTE [" << vi->key().ToString() << "]" << std::endl;
#endif
		m_storage->deleteIndex( vi->key());
	}
	// [1.4] Insert new attributes
	std::vector<DocAttribute>::const_iterator ai = m_attributes.begin(), ae = m_attributes.end();
	for (; ai != ae; ++ai)
	{
		docattribkey.resize( docattribkeysize);
		docattribkey.addPrefix( ai->name);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "PUT ATTRIBUTE [" << docattribkey << "]" << "= [" << wi->value << "]" << std::endl;
#endif
		leveldb::Slice keyslice( docattribkey.ptr(), docattribkey.size());
		m_storage->writeIndex( keyslice, ai->value);
	}

	// [2] Delete old document term occurrencies:
	std::set<TermMapKey> oldcontent;

	DatabaseKey invkey( (char)DatabaseKey::ForwardIndexPrefix, docno);
	std::size_t invkeysize = invkey.size();
	leveldb::Slice invkeyslice( invkey.ptr(), invkey.size());

	//[2.1] Iterate on key prefix elements [ForwardIndexPrefix, docno, typeno, *] and mark dem as deleted
	//	Extract typeno and valueno from key [ForwardIndexPrefix, docno, typeno, pos] an mark term as old content (do delete)
	for (vi->Seek( invkeyslice); vi->Valid(); vi->Next())
	{
		if (invkeysize > vi->key().size() || 0!=std::memcmp( vi->key().data(), invkey.ptr(), invkeysize))
		{
			//... end of document reached
			break;
		}
		documentFound = true;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "DELETE INV [" << vi->key().ToString() << "]" << std::endl;
#endif
		m_storage->deleteIndex( vi->key());

		const char* ki = vi->key().data() + invkeysize;
		const char* ke = ki + vi->key().size();
		Index typeno = unpackIndex( ki, ke);

		const char* valuestr = vi->value().data();
		std::size_t valuesize = vi->value().size();
		Index valueno = m_storage->keyLookUp( DatabaseKey::TermValuePrefix, std::string( valuestr, valuesize));

		oldcontent.insert( TermMapKey( typeno, valueno));
	}

	//[2.2] Iterate on 'oldcontent' elements built in [1.1] 
	//	and mark them as deleted the keys [InvertedIndexPrefix, typeno, valueno, docno]
	std::set<TermMapKey>::const_iterator di = oldcontent.begin(), de = oldcontent.end();
	for (; di != de; ++di)
	{
		DatabaseKey delkey( (char)DatabaseKey::InvertedIndexPrefix);
		delkey.addElem( di->first);		// [typeno]
		delkey.addElem( di->second);		// [valueno]
		delkey.addElem( docno);			// [docno]

		documentFound = true;
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "DELETE TERMS [" << delkey << "]" << std::endl;
#endif
		m_storage->deleteIndex( leveldb::Slice( delkey.ptr(), delkey.size()));

		m_storage->decrementDf( di->first, di->second);
	}

	//[3] Insert the new terms with key [InvertedIndexPrefix, typeno, valueno, docno]
	//	and value (weight as 32bit float, packed encoded difference of positions):
	TermMap::const_iterator ti = m_terms.begin(), te = m_terms.end();
	for (; ti != te; ++ti)
	{
		DatabaseKey termkey( (char)DatabaseKey::InvertedIndexPrefix);
		std::string positions;
		termkey.addElem( ti->first.first);	// [typeno]
		termkey.addElem( ti->first.second);	// [valueno]
		termkey.addElem( docno);		// [docno]

		packIndex( positions, ti->second.pos.size());
		// ... first element is the ff in the document
		std::set<Index>::const_iterator pi = ti->second.pos.begin(), pe = ti->second.pos.end();
		for (; pi != pe; ++pi)
		{
			packIndex( positions, *pi);
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "PUT TERMS [" << termkey << "]" << "= [" << positions << "]" << std::endl;
#endif
		m_storage->writeIndex( leveldb::Slice( termkey.ptr(), termkey.size()), positions);
		m_storage->incrementDf( ti->first.first, ti->first.second);

		m_storage->defineDocnoPosting(
				ti->first.first, ti->first.second,
				docno, ti->second.pos.size(),
				ti->second.weight);
	}

	// [4] Insert the new inverted info with key [ForwardIndexPrefix, docno, typeno, pos]:
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
		m_storage->writeIndex( invkeyslice, ri->second);
	}
	if (!documentFound)
	{
		m_storage->incrementNofDocumentsInserted();
	}

	// [5] Do submit the write to the database:
	m_storage->checkFlush();
	m_storage->releaseInserter();
}

