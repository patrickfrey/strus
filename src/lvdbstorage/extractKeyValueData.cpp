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
			else if (str[ii] == '\\') 
			{
				rt.append( "\\\\");
			}
			else
			{
				rt.push_back( str[ii]);
			}
		}
		else
		{
			rt.push_back( str[ii]);
		}
	}
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


InvertedIndexData::InvertedIndexData( const leveldb::Slice& key, const leveldb::Slice& value)
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
	ff = strus::unpackIndex( vi, ve);
	posar = new unsigned int[ ff];
	strus::Index prevpos = 0;
	strus::Index poscnt = 0;

	while (vi != ve)
	{
		strus::Index pos = strus::unpackIndex( vi, ve);
		if (pos > (strus::Index)std::numeric_limits<unsigned int>::max())
		{
			delete [] posar;
			throw std::runtime_error( "position out of range");
		}
		if (prevpos >= pos)
		{
			delete [] posar;
			throw std::runtime_error( "positions not ascending in location index");
		}
		if (poscnt > ff)
		{
			delete [] posar;
			throw std::runtime_error( "ff does not match to count of positions");
		}
		posar[ poscnt++] = (unsigned int)(prevpos = pos);
	}
	if (ff != poscnt)
	{
		delete [] posar;
		throw std::runtime_error( "ff does not match to count of positions");
	}
}

InvertedIndexData::~InvertedIndexData()
{
	if (posar) delete [] posar;
}

void InvertedIndexData::print( std::ostream& out)
{
	out << (char)DatabaseKey::InvertedIndexPrefix << ' ' << typeno << ' ' << valueno << ' ' << docno << ' ' << ff << ' ';
	unsigned int ii = 0;
	if (ff) out << posar[0];
	for (++ii; ii<ff; ++ii) out << ' ' << posar[ii];
	out << std::endl;
}


ForwardIndexData::ForwardIndexData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	pos = strus::unpackIndex( ki, ke);/*[pos]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of forward index key");
	}
	valuestr = vi;
	valuesize = ve-vi;
	if (!strus::checkStringUtf8( vi, ve-vi))
	{
		throw std::runtime_error( "value in addressed by forward index is not a valied UTF-8 string");
	}
}


void ForwardIndexData::print( std::ostream& out)
{
	out << (char)DatabaseKey::ForwardIndexPrefix << ' ' << docno << ' ' << typeno << ' ' << pos << ' ' << escapestr( valuestr, valuesize) << std::endl;
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
	out << (char)DatabaseKey::VariablePrefix << ' ' << valueno << ' ' << escapestr( varnamestr, varnamesize) << std::endl;
}


DocMetaDataData::DocMetaDataData( const leveldb::Slice& key, const leveldb::Slice& value)
{
	char const* ki = key.data()+1;
	char const* ke = key.data()+key.size();
	char const* vi = value.data();
	char const* ve = value.data()+value.size();

	if (ki == ke)
	{
		throw std::runtime_error( "unexpected end of metadata key");
	}
	name = *ki++;
	if (name < 32 || name > 127)
	{
		throw std::runtime_error( "variable name in metadata key out of range");
	}
	blockno = strus::unpackIndex( ki, ke);/*[blockno]*/
	if (ki != ke)
	{
		throw std::runtime_error( "unexpected extra bytes at end of metadata key");
	}
	blk = reinterpret_cast<const float*>(vi);
	blksize = (ve - vi) / sizeof(float);
	if (blksize != (strus::MetaDataBlock::MetaDataBlockSize))
	{
		throw std::runtime_error( "corrupt meta data block (unexpected size of meta data block)");
	}
}

void DocMetaDataData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocMetaDataPrefix << ' ' << name << ' ' << blockno;
	unsigned int ii = 0;
	out << blk[0];
	for (++ii; ii<blksize; ++ii) out << ' ' << blk[ii];
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
	name = *ki++;
	if (name < 32 || name > 127)
	{
		throw std::runtime_error( "variable name in document attribute key out of range");
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
	out << (char)DatabaseKey::DocAttributePrefix << ' ' << name << ' ' << docno << ' ' << escapestr( valuestr, valuesize) << std::endl;
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

	blk = reinterpret_cast<const DocnoBlock::Element*>(vi);
	blksize = (ve-vi)/sizeof(DocnoBlock::Element);
	if (blksize == 0)
	{
		throw std::runtime_error( "docno block is empty");
	}
	if (blksize * sizeof(DocnoBlock::Element) != (std::size_t)(ve-vi))
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


