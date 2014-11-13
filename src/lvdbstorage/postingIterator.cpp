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
#include "postingIterator.hpp"
#include "storage.hpp"
#include "indexPacker.hpp"
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <iostream>

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG

#ifdef STRUS_LOWLEVEL_DEBUG
PostingIterator::PostingIterator( leveldb::DB* db_, Index termtypeno, Index termvalueno, const char* termstr)
#else
PostingIterator::PostingIterator( leveldb::DB* db_, Index termtypeno, Index termvalueno, const char*)
#endif
	:m_docnoitr(m_db,termtypeno,termvalueno)
	,m_db(db_)
	,m_termtypeno(termtypeno)
	,m_termvalueno(termvalueno)
	,m_key( (char)DatabaseKey::InvertedIndexPrefix, termtypeno, termvalueno)
	,m_keysize(1)
	,m_docno(0)
	,m_lastdocno(0)
	,m_documentFrequency(-1)
	,m_itr(0)
	,m_posno(0)
	,m_positr(0)
	,m_posend(0)
{
	m_featureid.reserve( 16);
	m_keysize = m_key.size();
#ifdef STRUS_LOWLEVEL_DEBUG
	m_featureid.append( termstr);
	m_featureid.push_back(':');
	m_featureid.push_back( (char)(termtypeno/10) + '0');
	m_featureid.push_back( (char)(termtypeno%10) + '0');
#else
	m_featureid.append( m_key.ptr()+1, m_keysize-1);
#endif
}

PostingIterator::PostingIterator( const PostingIterator& o)
	:PostingIteratorInterface(o)
	,m_docnoitr(o.m_db,o.m_termtypeno,o.m_termvalueno)
	,m_db(o.m_db)
	,m_termtypeno(o.m_termtypeno)
	,m_termvalueno(o.m_termvalueno)
	,m_key(o.m_key)
	,m_keysize(o.m_keysize)
	,m_docno(0)
	,m_lastdocno(0)
	,m_documentFrequency(o.m_documentFrequency)
	,m_itr(0)
	,m_posno(0)
	,m_positr(0)
	,m_posend(0)
	,m_featureid(o.m_featureid)
{}

PostingIterator::~PostingIterator()
{
	if (m_itr) delete m_itr;
}


Index PostingIterator::skipDoc( const Index& docno_)
{
	if (m_docno)
	{
		if (m_docno == docno_)
		{
			return m_docno;
		}
		if (m_lastdocno <= docno_ && m_docno >= docno_)
		{
			return m_docno;
		}
	}
	Index dn = m_docnoitr.skipDoc( docno_);
	if (dn)
	{
		m_lastdocno = docno_;
		m_docno = dn;
	}
	return dn;
#if 0
	if (m_itr && m_docno +1 == docno_)
	{
		return getNextTermDoc();
	}
	else
	{
		return getFirstTermDoc( docno_);
	}
#endif
}

Index PostingIterator::skipPos( const Index& firstpos_)
{
#if 1
	if (!getFirstTermDoc( m_docno)) return 0;
#endif
	if (m_posno >= firstpos_)
	{
		if (m_posno == firstpos_ && firstpos_ != 0)
		{
			return m_posno;
		}
		m_posno = 0;
		m_positr = m_itr->value().data();
		m_posend = m_positr + m_itr->value().size();
		m_positr = skipIndex( m_positr, m_posend); //... skip ff
	}
	unsigned int ofs = (m_posend - m_positr) >> 1;
	if (ofs > firstpos_ - m_posno)
	{
		ofs = (firstpos_ - m_posno) >> 4;
	}
	while (ofs >= 6)
	{
		const char* skipitr = strus::nextPackedIndexPos( m_positr, m_positr + ofs, m_posend);
		if (skipitr != m_posend)
		{
			Index nextpos = unpackIndex( skipitr, m_posend);
			if (nextpos <= firstpos_)
			{
				m_posno = nextpos;
				m_positr = skipitr;
				if (nextpos == firstpos_)
				{
					return m_posno;
				}
				else
				{
					ofs = (m_posend - m_positr) >> 1;
					if (ofs > firstpos_ - m_posno)
					{
						ofs = (firstpos_ - m_posno) >> 4;
					}
					continue;
				}
			}
			else
			{
				ofs >>= 1;
			}
		}
	}
	while (m_positr < m_posend && (firstpos_ > m_posno || !m_posno))
	{
		// Get the next position:
		m_posno = unpackIndex( m_positr, m_posend);
	}
	if (firstpos_ > m_posno)
	{
		return 0;
	}
	return m_posno;
}

Index PostingIterator::extractMatchData()
{
	if (m_itr->Valid()
	&& m_keysize < m_itr->key().size()
	&& 0==std::memcmp( m_key.ptr(), m_itr->key().data(), m_keysize))
	{
		// Init the term weight and the iterators on the term occurrencies:
		m_posno = 0;
		m_positr = m_itr->value().data();
		m_posend = m_positr + m_itr->value().size();
		(void)unpackIndex( m_positr, m_posend);/*ff*/

		// Extract the next matching document number from the rest of the key and return it:
		const char* ki = m_itr->key().data() + m_keysize;
		const char* ke = ki + m_itr->key().size();
		m_docno = unpackIndex( ki, ke);
		return m_docno;
	}
	else
	{
		delete m_itr;
		m_docno = 0;
		m_itr = 0;
		m_posno = 0;
		m_positr = 0;
		m_posend = 0;
		return 0;
	}
}

Index PostingIterator::getNextTermDoc()
{
	m_itr->Next();
	return extractMatchData();
}

Index PostingIterator::getFirstTermDoc( const Index& docno)
{
	if (!m_itr)
	{
		leveldb::ReadOptions options;
		options.fill_cache = true;
		m_itr = m_db->NewIterator( options);
	}
	m_key.resize( m_keysize);
	m_key.addElem( docno);
	m_itr->Seek( leveldb::Slice( m_key.ptr(), m_key.size()));

	return extractMatchData();
}

Index PostingIterator::documentFrequency()
{
	if (m_documentFrequency < 0)
	{
		DatabaseKey key( (char)DatabaseKey::DocFrequencyPrefix, m_termtypeno, m_termvalueno);
		leveldb::Slice keyslice( key.ptr(), key.size());
		std::string value;
		leveldb::Status status = m_db->Get( leveldb::ReadOptions(), keyslice, &value);
		if (status.IsNotFound())
		{
			return 0;
		}
		if (!status.ok())
		{
			throw std::runtime_error( status.ToString());
		}
		const char* cc = value.c_str();
		m_documentFrequency = unpackIndex( cc, cc + value.size());
	}
	return m_documentFrequency;
}

