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
#include "extractKeyValueData.hpp"
#include "indexPacker.hpp"
#include "metaDataBlock.hpp"
#include "forwardIndexBlock.hpp"
#include "booleanBlock.hpp"
#include "strus/arithmeticVariant.hpp"
#include <stdexcept>
#include <iostream>
#include <limits>

using namespace strus;

TermTypeData::TermTypeData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	typestr = ki;
	typesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw std::runtime_error( "key of term type is not a valid UTF8 string");
	}
	typeno = strus::unpackIndex( vi, ve);/*[typeno]*/
	if (vi != ve)
	{
		throw std::runtime_error( "unexpected extra bytes at end of term type number");
	}
}

static std::string escapestr( const char* str, std::size_t size)
{
	std::string rt;
	rt.reserve( 128);
	rt.push_back('"');

	std::size_t ii=0;
	for (; ii<size; ++ii)
	{
		if (str[ii] < 32)
		{
			if (str[ii] == '\n')
			{
				rt.append( "\\n");
			}
			else if (str[ii] == '\r') 
			{
				rt.append( "\\r");
			}
			else if (str[ii] == '\b') 
			{
				rt.append( "\\b");
			}
			else if (str[ii] == '\t') 
			{
				rt.append( "\\t");
			}
			else
			{
				rt.push_back( str[ii]);
			}
		}
		else if (str[ii] == '\"') 
		{
			rt.append( "\\\"");
		}
		else if (str[ii] == '\\') 
		{
			rt.append( "\\\\");
		}
		else
		{
			rt.push_back( str[ii]);
		}
	}
	rt.push_back('"');
	return rt;
}

void TermTypeData::print( std::ostream& out)
{
	out << (char)DatabaseKey::TermTypePrefix << ' ' << typeno << ' ' << escapestr( typestr, typesize) << std::endl;
}


TermValueData::TermValueData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	valuestr = ki;
	valuesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw std::runtime_error( "key of term value is not a valid UTF8 string");
	}
	valueno = strus::unpackIndex( vi, ve);/*[valueno]*/
	if (vi != ve)
	{
		throw std::runtime_error( "unexpected extra bytes at end of term value number");
	}
}

void TermValueData::print( std::ostream& out)
{
	out << (char)DatabaseKey::TermValuePrefix << ' ' << valueno << ' ' << escapestr( valuestr, valuesize) << std::endl;
}



DocIdData::DocIdData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	docidstr = ki;
	docidsize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw std::runtime_error( "key of doc id is not a valid UTF8 string");
	}
	docno = strus::unpackIndex( vi, ve);/*[docno]*/
	if (vi != ve)
	{
		throw std::runtime_error( "unexpected extra bytes at end of document number");
	}
}

void DocIdData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocIdPrefix << ' ' << docno << ' ' << escapestr( docidstr, docidsize) << std::endl;
}


UserNameData::UserNameData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	usernamestr = ki;
	usernamesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw std::runtime_error( "key of doc id is not a valid UTF8 string");
	}
	userno = strus::unpackIndex( vi, ve);/*[userno]*/
	if (vi != ve)
	{
		throw std::runtime_error( "unexpected extra bytes at end of user number");
	}
}

void UserNameData::print( std::ostream& out)
{
	out << (char)DatabaseKey::UserNamePrefix << ' ' << userno << ' ' << escapestr( usernamestr, usernamesize) << std::endl;
}



ForwardIndexData::ForwardIndexData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	pos = strus::unpackIndex( ki, ke);/*[pos]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of forward index key");
	}
	ForwardIndexBlock blk( pos, vi, ve-vi);
	const char* bi = blk.charptr();
	const char* be = blk.charend();
	Index prevpos = 0;
	for (; bi != be; bi = blk.nextItem( bi))
	{
		std::string value( blk.value_at( bi));
		if (!strus::checkStringUtf8( value.c_str(), value.size()))
		{
			throw std::runtime_error( "value in addressed by forward index is not a valid UTF-8 string");
		}
		Index curpos = blk.position_at(bi);
		if (curpos <= prevpos)
		{
			throw std::runtime_error( "positions in forward index are not in strictly ascending order");
		}
		if (curpos > pos)
		{
			throw std::runtime_error( "position found in forward index that is bigger than the block id");
		}
		elements.push_back( Element( curpos, value));
	}
}

void ForwardIndexData::print( std::ostream& out)
{
	out << (char)DatabaseKey::ForwardIndexPrefix << ' ' << typeno << ' ' << docno << ' ' << pos;
	std::vector<ForwardIndexData::Element>::const_iterator
		ei = elements.begin(), ee = elements.end();

	for (; ei != ee; ++ei)
	{
		out << ' ' << ei->pos
			<< ' '
			<< escapestr( ei->value.c_str(), ei->value.size());
	}
	out << std::endl;
}


VariableData::VariableData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	varnamestr = ki;
	varnamesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw std::runtime_error( "illegal UTF8 string as key of global variable");
	}
	valueno = strus::unpackIndex( vi, ve);/*[value]*/
	if (vi != ve)
	{
		throw std::runtime_error( "unexpected extra bytes at end of variable value");
	}
}	

void VariableData::print( std::ostream& out)
{
	out << (char)DatabaseKey::VariablePrefix << ' ' << escapestr( varnamestr, varnamesize) << ' ' << valueno << std::endl;
}


DocMetaDataData::DocMetaDataData( const MetaDataDescription* metadescr, const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	if (ki == ke)
	{
		throw std::runtime_error( "unexpected end of metadata key");
	}
	blockno = strus::unpackIndex( ki, ke);/*[blockno]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of metadata block key");
	}
	block.init( metadescr, blockno, vi, ve-vi);
	descr = metadescr;
}

void DocMetaDataData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocMetaDataPrefix << ' ' << blockno;
	unsigned int ii = 0;
	for (; ii<MetaDataBlock::BlockSize; ++ii)
	{
		if (ii) out << ' ';
		MetaDataRecord record = block[ ii];

		unsigned int colidx = 0, colend = 0;
		for (; colidx<colend; ++colidx)
		{
			if (colidx) out << ',';
			ArithmeticVariant value = record.getValue( descr->get( colidx));
			out << value;
		}
	}
	out << std::endl;
}


DocAttributeData::DocAttributeData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki == ke)
	{
		throw std::runtime_error( "unexpected end of document attribute key");
	}
	attribno = strus::unpackIndex( ki, ke); /*[attribno]*/
	if (ki != ke)
	{
		throw std::runtime_error( "extra character at end of document attribute key");
	}
	valuestr = vi;
	valuesize = ve-vi;
	if (!strus::checkStringUtf8( vi, ve-vi))
	{
		throw std::runtime_error( "value in document attribute value is not a valid UTF-8 string");
	}
}	

void DocAttributeData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocAttributePrefix << ' ' << attribno << ' ' << docno << ' ' << escapestr( valuestr, valuesize) << std::endl;
}


DocFrequencyData::DocFrequencyData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	termno = strus::unpackIndex( ki, ke);/*[valueno]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of term document frequency key");
	}
	df = strus::unpackIndex( vi, ve);/*[df]*/
	if (vi != ve)
	{
		throw std::runtime_error( "unexpected extra bytes at end of df value");
	}
}

void DocFrequencyData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocFrequencyPrefix << ' ' << typeno << ' ' << termno << ' ' << df << std::endl;
}


DocnoBlockData::DocnoBlockData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	termno = strus::unpackIndex( ki, ke);/*[termno]*/
	Index docno = strus::unpackIndex( ki, ke);/*[docno]*/

	blk = reinterpret_cast<const DocnoBlockElement*>(vi);
	blksize = (ve-vi)/sizeof(DocnoBlockElement);
	if (blksize == 0)
	{
		throw std::runtime_error( "docno block is empty");
	}
	if (blksize * sizeof(DocnoBlockElement) != (std::size_t)(ve-vi))
	{
		throw std::runtime_error( "docno block size is not dividable by docno block element size");
	}
	if (blk[blksize-1].docno() != docno)
	{
		throw std::runtime_error( "last docno block element does not match to key");
	}
}

void DocnoBlockData::print( std::ostream& out)
{
	std::size_t ii=0;
	out << (char)DatabaseKey::DocnoBlockPrefix;
	out << ' ' << typeno << ' ' << termno;
	for (; ii<blksize; ++ii)
	{
		out << ' ' << blk[ii].docno() << ',' << blk[ii].ff() << ',' << blk[ii].weight();
	}
	out << std::endl;
}


PosinfoBlockData::PosinfoBlockData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	valueno = strus::unpackIndex( ki, ke);/*[valueno]*/
	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of term index key");
	}
	PosinfoBlock blk( docno, vi, ve-vi);
	char const* itr = blk.begin();
	char const* end = blk.end();

	while (itr != end)
	{
		Index docno_elem = blk.docno_at( itr);
		std::size_t ff = (std::size_t)blk.frequency_at( itr);
		if (docno_elem > docno)
		{
			throw std::runtime_error( "posinfo element docno bigger than upper bound docno");
		}
		if (posinfo.size() && docno_elem <= posinfo.back().docno)
		{
			throw std::runtime_error( "elements in posinfo array of docno not not strictly ascending");
		}
		posinfo.push_back( PosinfoPosting( docno_elem, blk.positions_at( itr)));
		if (ff != posinfo.back().pos.size() && posinfo.back().pos.size() != 0)
		{
			throw std::runtime_error( "ff does not match to length of position array");
		}
		std::vector<Index>::const_iterator pi = posinfo.back().pos.begin(), pe = posinfo.back().pos.end();
		for (Index prevpos=0; pi != pe; ++pi)
		{
			if (prevpos >= *pi)
			{
				throw std::runtime_error( "position elements in posinfo array not strictly ascending");
			}
		}
		itr = blk.nextDoc( itr);
	}
}

void PosinfoBlockData::print( std::ostream& out)
{
	out << (char)DatabaseKey::PosinfoBlockPrefix << ' ' << typeno << ' ' << valueno << ' ' << posinfo.size();
	std::vector<PosinfoPosting>::const_iterator itr = posinfo.begin(), end = posinfo.end();
	for (; itr != end; ++itr)
	{
		out << ' ' << itr->docno << ':';
		std::vector<Index>::const_iterator pi = itr->pos.begin(), pe = itr->pos.end();
		for (int pidx=0; pi != pe; ++pi,++pidx)
		{
			if (pidx) out << ',';
			out << *pi;
		}
	}
	out << std::endl;
}


static std::vector<std::pair<Index,Index> > getRangeListFromBooleanBlock(
		DatabaseKey::KeyPrefix prefix, const Index& id, char const* vi, const char* ve)
{
	std::vector<std::pair<Index,Index> > rt;
	BooleanBlock blk( prefix, id, vi, ve-vi);
	char const* itr = blk.charptr();

	Index rangemin;
	Index rangemax;
	Index prevmax = 0;

	while (blk.getNextRange( itr, rangemin, rangemax))
	{
		if (rangemax <= 0)
		{
			throw std::runtime_error( "illegal range in boolean block (negative or zero maximum)");
		}
		if (rangemin <= 0)
		{
			throw std::runtime_error( "illegal range in boolean block (negative or zero minimum)");
		}
		if (rangemin > rangemax)
		{
			throw std::runtime_error( "illegal range in boolean block (min > max)");
		}
		if (rangemax > id)
		{
			throw std::runtime_error( "illegal range in boolean block (max > blockId)");
		}
		if (rangemin <= prevmax)
		{
			throw std::runtime_error( "illegal range in boolean block (not strictly ascending or unjoined overlapping ranges)");
		}
		prevmax = rangemax;
		rt.push_back( std::pair<Index,Index>( rangemin, rangemax));
	}
	return rt;
}

static void printRangeList( std::ostream& out, const std::vector<std::pair<Index,Index> >& rangelist)
{
	std::vector<std::pair<Index,Index> >::const_iterator itr = rangelist.begin(), end = rangelist.end();
	for (; itr != end; ++itr)
	{
		if (itr->first == itr->second)
		{
			out << ' ' << itr->first;
		}
		else
		{
			out << ' ' << itr->first << ".." << itr->second;
		}
	}
}

DocListBlockData::DocListBlockData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	valueno = strus::unpackIndex( ki, ke);/*[valueno]*/
	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of term index key");
	}
	docrangelist = getRangeListFromBooleanBlock( DatabaseKey::DocListBlockPrefix, docno, vi, ve);
}

void DocListBlockData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocListBlockPrefix << ' ' << typeno << ' ' << valueno;
	printRangeList( out, docrangelist);
	out << std::endl;
}


typedef InvTermBlock::Element InvTerm;
std::vector<InvTerm> terms;

InverseTermData::InverseTermData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of inverse term index key");
	}

	InvTermBlock block( docno, vi, ve-vi);
	char const* ii = block.begin();
	const char* ie = block.end();
	for (; ii != ie; ii=block.next(ii))
	{
		terms.push_back( block.element_at( ii));
	}
}

void InverseTermData::print( std::ostream& out)
{
	out << (char)DatabaseKey::InverseTermIndex << ' ' << docno;
	std::vector<InvTerm>::const_iterator ti = terms.begin(), te = terms.end();
	for (; ti != te; ++ti)
	{
		out << ' ' << ti->typeno << ',' << ti->termno << ',' << ti->df;
	}
	out << std::endl;
}


UserAclBlockData::UserAclBlockData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	userno = strus::unpackIndex( ki, ke);/*[userno]*/
	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of user index key");
	}
	docrangelist = getRangeListFromBooleanBlock( DatabaseKey::UserAclBlockPrefix, docno, vi, ve);
}

void UserAclBlockData::print( std::ostream& out)
{
	out << (char)DatabaseKey::UserAclBlockPrefix << ' ' << userno;
	printRangeList( out, docrangelist);
	out << std::endl;
}


AclBlockData::AclBlockData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	userno = strus::unpackIndex( ki, ke);/*[userno]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of docno index key");
	}
	userrangelist = getRangeListFromBooleanBlock( DatabaseKey::AclBlockPrefix, userno, vi, ve);
}

void AclBlockData::print( std::ostream& out)
{
	out << (char)DatabaseKey::AclBlockPrefix << ' ' << docno;
	printRangeList( out, userrangelist);
	out << std::endl;
}


AttributeKeyData::AttributeKeyData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	varnamestr = ki;
	varnamesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw std::runtime_error( "illegal UTF8 string as attribute key");
	}
	valueno = strus::unpackIndex( vi, ve);/*[value]*/
	if (vi != ve)
	{
		throw std::runtime_error( "unexpected extra bytes at end of attribute number");
	}
}

void AttributeKeyData::print( std::ostream& out)
{
	out << (char)DatabaseKey::AttributeKeyPrefix << ' ' << escapestr( varnamestr, varnamesize) << ' ' << valueno << std::endl;
}


MetaDataDescrData::MetaDataDescrData( const leveldb::Slice& key, const leveldb::Slice& value)
	:descr( value.data())
{
	if (key.size() != 1) std::runtime_error( "illegal (not empty) key for meta data description");
}


void MetaDataDescrData::print( std::ostream& out)
{
	std::vector<std::string> cols = descr.columns();
	std::vector<std::string>::const_iterator
		ci = cols.begin(), ce = cols.end();
	for (std::size_t cidx=0; ci != ce; ++ci,++cidx)
	{
		if (cidx) out << ' ';
		out << *ci;
	}
}


