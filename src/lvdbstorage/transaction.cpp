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
#include "storage.hpp"
#include "transaction.hpp"
#include "indexPacker.hpp"
#include <string>
#include <cstring>
#include <set>
#include <boost/thread/mutex.hpp>
#include <boost/scoped_ptr.hpp>

using namespace strus;

Transaction::Transaction( Storage* storage_, const std::string& docid_)
	:m_storage(storage_),m_docid(docid_)
{}

Transaction::~Transaction()
{
	//... nothing done here. The document id and term value or type strings 
	//	created might remain inserted, even after a rollback.
}

Transaction::TermMapKey Transaction::termMapKey( const std::string& type_, const std::string& value_)
{
	Index typeno = m_storage->keyGetOrCreate( Storage::TermTypePrefix, type_);
	Index valueno = m_storage->keyGetOrCreate( Storage::TermValuePrefix, value_);
	return TermMapKey( typeno, valueno);
}

void Transaction::addTermOccurrence(
		const std::string& type_,
		const std::string& value_,
		const Index& position_)
{
	if (position_ == 0) throw std::runtime_error( "term occurrence position must not be 0");

	TermMapKey key( termMapKey( type_, value_));
	m_terms[ key].pos.insert( position_);
	InvMapValue* inv = &m_invs[ position_];
	inv->typeno = key.first;
	inv->value = value_;
}

void Transaction::setTermWeight(
		const std::string& type_,
		const std::string& value_,
		float weight_)
{
	m_terms[ termMapKey( type_, value_)].weight = weight_;
}

void Transaction::setDocumentAttribute(
		char name_,
		float weight_)
{
	std::string value;
	packFloat( value, weight_);
	m_attributes.push_back( DocAttribute( DocAttribute::TypeNumber, name_, value));
}

void Transaction::setDocumentAttribute(
		char name_,
		const std::string& value_)
{
	m_attributes.push_back( DocAttribute( DocAttribute::TypeString, name_, value_));
}

void Transaction::commit()
{
	Index docno = m_storage->keyGetOrCreate( Storage::DocIdPrefix, m_docid);
	leveldb::WriteBatch batch;
	bool documentFound = false;

	leveldb::Iterator* vi = m_storage->newIterator();
	boost::scoped_ptr<leveldb::Iterator> viref(vi);

	// [1] Delete old document attributes and insert the new ones
	std::string weightkey;
	std::size_t weightkeysize;

	// [1.1] Delete old numeric attributes
	weightkey.push_back( (char)Storage::DocNumAttrPrefix);
	packIndex( weightkey, docno);
	weightkeysize = weightkey.size();

	for (vi->Seek( weightkey); vi->Valid(); vi->Next())
	{
		if (weightkey.size() > vi->key().size() || 0!=std::strcmp( vi->key().data(), weightkey.c_str()))
		{
			//... end of document reached
			break;
		}
		documentFound = true;
		batch.Delete( vi->key());
	}
	// [1.2] Insert new numeric attributes
	std::vector<DocAttribute>::const_iterator wi = m_attributes.begin(), we = m_attributes.end();
	for (; wi != we; ++wi)
	{
		if (wi->type == DocAttribute::TypeNumber)
		{
			weightkey.push_back( wi->name);
			batch.Put( weightkey, wi->value);
			weightkey.resize( weightkeysize);
		}
	}

	// [1.3] Delete old textual attributes
	weightkey.clear();
	weightkey.push_back( (char)Storage::DocTextAttrPrefix);
	packIndex( weightkey, docno);
	weightkeysize = weightkey.size();

	for (vi->Seek( weightkey); vi->Valid(); vi->Next())
	{
		if (weightkey.size() > vi->key().size() || 0!=std::strcmp( vi->key().data(), weightkey.c_str()))
		{
			//... end of document reached
			break;
		}
		documentFound = true;
		batch.Delete( vi->key());
	}
	// [1.4] Insert new textual attributes
	wi = m_attributes.begin(), we = m_attributes.end();
	for (; wi != we; ++wi)
	{
		if (wi->type == DocAttribute::TypeString)
		{
			weightkey.push_back( wi->name);
			batch.Put( weightkey, wi->value);
			weightkey.resize( weightkeysize);
		}
	}

	// [2] Delete old document term occurrencies:
	std::set<TermMapKey> oldcontent;

	std::string invkey;
	std::size_t invkeysize;
	invkey.push_back( (char)Storage::InversePrefix);
	packIndex( invkey, docno);
	invkeysize = invkey.size();

	//[2.1] Iterate on key prefix elements [InversePrefix, docno, typeno, *] and mark dem as deleted
	//	Extract typeno and valueno from key [InversePrefix, docno, typeno, pos] an mark term as old content (do delete)
	for (vi->Seek( invkey); vi->Valid(); vi->Next())
	{
		if (invkey.size() > vi->key().size() || 0!=std::strcmp( vi->key().data(), invkey.c_str()))
		{
			//... end of document reached
			break;
		}
		documentFound = true;
		batch.Delete( vi->key());

		const char* ki = vi->key().data() + invkeysize;
		const char* ke = ki + vi->key().size();
		Index typeno = unpackIndex( ki, ke);

		const char* valuestr = vi->value().data();
		std::size_t valuesize = vi->value().size();
		Index valueno = m_storage->keyLookUp( Storage::TermValuePrefix, std::string( valuestr, valuesize));

		oldcontent.insert( TermMapKey( typeno, valueno));
	}
	//[2.2] Iterate on oldcontent elements built in [1.1] 
	//	and mark them as deleted the keys [LocationPrefix, typeno, valueno, docno]
	std::set<TermMapKey>::const_iterator di = oldcontent.begin(), de = oldcontent.end();
	for (; di != de; ++di)
	{
		std::string delkey;
		delkey.push_back( (char)Storage::LocationPrefix);
		packIndex( delkey, di->first);		// [typeno]
		packIndex( delkey, di->second);		// [valueno]
		packIndex( delkey, docno);		// [docno]

		documentFound = true;
		batch.Delete( delkey);
	}

	//[3] Insert the new terms with key [LocationPrefix, typeno, valueno, docno]
	//	and value (weight as 32bit float, packed encoded difference of positions):
	TermMap::const_iterator ti = m_terms.begin(), te = m_terms.end();
	for (; ti != te; ++ti)
	{
		std::string termkey;
		std::string positions;
		termkey.push_back( (char)Storage::LocationPrefix);
		packIndex( termkey, ti->first.first);	// [typeno]
		packIndex( termkey, ti->first.second);	// [valueno]
		packIndex( termkey, docno);		// [docno]

		std::set<Index>::const_iterator pi = ti->second.pos.begin(), pe = ti->second.pos.end();
		float weight = ti->second.weight;
		positions.append( reinterpret_cast<const char*>(&weight), sizeof(weight));

		for (; pi != pe; ++pi)
		{
			packIndex( positions, *pi);
		}
		batch.Put( termkey, positions);
	}

	// [4] Insert the new inverted info with key [InversePrefix, docno, typeno, pos]:
	InvMap::const_iterator ri = m_invs.begin(), re = m_invs.end();
	for (; ri != re; ++ri)
	{
		invkey.clear();
		invkey.push_back( (char)Storage::InversePrefix);
		packIndex( invkey, docno);		//docno
		packIndex( invkey, ri->second.typeno);	//term typeno
		packIndex( invkey, ri->first);		//position

		batch.Put( invkey, ri->second.value);
	}
	if (!documentFound)
	{
		m_storage->incrementNofDocumentsInserted();
	}

	// [5] Do submit the write to the database:
	m_storage->writeBatch( batch);
	m_storage->flushNewKeys();
	batch.Clear();
}

