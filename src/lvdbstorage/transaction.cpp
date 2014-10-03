/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#include <boost/thread/mutex.hpp>

using namespace strus;

Transaction::Transaction( Storage* storage_, const std::string& docid_)
	:m_storage(storage_),m_docid(docid_)
{
}

Transaction::~Transaction()
{
	//... nothing done here. The document id and term value or type strings 
	//	created might remain inserted, even after a rollback.
}

void Transaction::addTermOccurrence(
		const std::string& type_,
		const std::string& value_,
		const Index& position_)
{
	if (position_ == 0) throw std::runtime_error( "term occurrence position must not be 0");

	Index typeno = m_storage->keyGetOrCreate( Storage::TermTypePrefix, type_);
	Index valueno = m_storage->keyGetOrCreate( Storage::TermValuePrefix, value_);

	std::vector<Index>* termpos = &m_terms[ TermMapKey( typeno, valueno)];
	if (termpos->size())
	{
		if (termpos->back() == position_)
		{
			return; // ... ignoring multiple matches
		}
	}
	else if (termpos->back() < position_)
	{
		termpos->push_back( position_);
	}
	else
	{
		std::string* encterm = &m_invs[ position_];
		packIndex( *encterm, typeno);
		packIndex( *encterm, valueno);

		std::vector<Index>::iterator pi = termpos->begin(), pe = termpos->end();
		for (; pi != pe; ++pi)
		{
			if (*pi >= position_)
			{
				if (*pi > position_)
				{
					// ... ignoring (*pi == position_) multiple matches
					// inserting match:
					termpos->insert( pi, position_);
				}
				return;
			}
		}
	}
}

void Transaction::commit()
{
	Index docno = m_storage->keyGetOrCreate( Storage::DocIdPrefix, m_docid);
	leveldb::WriteBatch batch;

	// Delete old document term occurrencies:
	std::map< TermMapKey, bool > oldcontent;
	std::string invkey;
	invkey.push_back( (char)Storage::InversePrefix);
	packIndex( invkey, docno);

	leveldb::Iterator* vi = m_storage->newIterator();
	for (vi->Seek( invkey); vi->Valid(); vi->Next())
	{
		if (invkey.size() > vi->key().size() || 0!=std::strcmp( vi->key().data(), invkey.c_str()))
		{
			//... end of document reached
			break;
		}
		batch.Delete( vi->key());

		const char* di = vi->value().data();
		const char* de = di + vi->value().size();
		while (di != de)
		{
			Index typeno = unpackIndex( di, de);
			Index termno = unpackIndex( di, de);
			oldcontent[ TermMapKey( typeno, termno)] = true;
		}
	}
	std::map< TermMapKey, bool >::const_iterator di = oldcontent.begin(), de = oldcontent.end();
	for (; di != de; ++di)
	{
		std::string delkey;
		delkey.push_back( (char)Storage::LocationPrefix);
		packIndex( delkey, di->first.first);
		packIndex( delkey, di->first.second);
		packIndex( delkey, docno);
		batch.Delete( delkey);
	}

	// Insert the new terms:
	TermMap::const_iterator ti = m_terms.begin(), te = m_terms.end();
	for (; ti != te; ++ti)
	{
		std::string termkey;
		std::string positions;
		termkey.push_back( (char)Storage::LocationPrefix);
		packIndex( termkey, ti->first.first);
		packIndex( termkey, ti->first.second);
		packIndex( termkey, docno);
		std::vector<Index>::const_iterator pi = ti->second.begin(), pe = ti->second.end();
		Index previous_pos = 0;
		for (; pi != pe; ++pi)
		{
			packIndex( positions, *pi - previous_pos);
			previous_pos = *pi;
		}
		batch.Put( termkey, positions);
	}

	// Insert the new inverted info:
	InvMap::const_iterator ri = m_invs.begin(), re = m_invs.end();
	for (; ri != re; ++ri)
	{
		invkey.clear();
		invkey.push_back( (char)Storage::InversePrefix);
		packIndex( invkey, docno);
		packIndex( invkey, ri->first);

		batch.Put( invkey, ri->second);
	}

	// Do submit the write to the database:
	m_storage->writeBatch( batch);
}

