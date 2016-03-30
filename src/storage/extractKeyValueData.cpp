/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "extractKeyValueData.hpp"
#include "indexPacker.hpp"
#include "databaseKey.hpp"
#include "metaDataBlock.hpp"
#include "forwardIndexBlock.hpp"
#include "booleanBlock.hpp"
#include "strus/numericVariant.hpp"
#include "private/internationalization.hpp"
#include <stdexcept>
#include <iostream>
#include <limits>

using namespace strus;

TermTypeData::TermTypeData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	typestr = ki;
	typesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw strus::runtime_error( _TXT( "key of term type is not a valid UTF8 string"));
	}
	typeno = strus::unpackIndex( vi, ve);/*[typeno]*/
	if (vi != ve)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of term type number"));
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


TermValueData::TermValueData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	valuestr = ki;
	valuesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw strus::runtime_error( _TXT( "key of term value is not a valid UTF8 string"));
	}
	valueno = strus::unpackIndex( vi, ve);/*[valueno]*/
	if (vi != ve)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of term value number"));
	}
}

void TermValueData::print( std::ostream& out)
{
	out << (char)DatabaseKey::TermValuePrefix << ' ' << valueno << ' ' << escapestr( valuestr, valuesize) << std::endl;
}



DocIdData::DocIdData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	docidstr = ki;
	docidsize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw strus::runtime_error( _TXT( "key of doc id is not a valid UTF8 string"));
	}
	docno = strus::unpackIndex( vi, ve);/*[docno]*/
	if (vi != ve)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of document number"));
	}
}

void DocIdData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocIdPrefix << ' ' << docno << ' ' << escapestr( docidstr, docidsize) << std::endl;
}


UserNameData::UserNameData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	usernamestr = ki;
	usernamesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw strus::runtime_error( _TXT( "key of doc id is not a valid UTF8 string"));
	}
	userno = strus::unpackIndex( vi, ve);/*[userno]*/
	if (vi != ve)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of user number"));
	}
}

void UserNameData::print( std::ostream& out)
{
	out << (char)DatabaseKey::UserNamePrefix << ' ' << userno << ' ' << escapestr( usernamestr, usernamesize) << std::endl;
}

TermTypeInvData::TermTypeInvData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	typestr = vi;
	typesize = ve-vi;
	if (!strus::checkStringUtf8( vi, ve-vi))
	{
		throw strus::runtime_error( _TXT( "value of term type is not a valid UTF8 string"));
	}
	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of term type number"));
	}
}

void TermTypeInvData::print( std::ostream& out)
{
	out << (char)DatabaseKey::TermTypeInvPrefix << ' ' << escapestr( typestr, typesize) << ' ' << typeno << std::endl;
}

TermValueInvData::TermValueInvData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	valuestr = vi;
	valuesize = ve-vi;
	if (!strus::checkStringUtf8( vi, ve-vi))
	{
		throw strus::runtime_error( _TXT( "value of term is not a valid UTF8 string"));
	}
	valueno = strus::unpackIndex( ki, ke);/*[valueno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of term number"));
	}
}

void TermValueInvData::print( std::ostream& out)
{
	out << (char)DatabaseKey::TermValueInvPrefix << ' ' << escapestr( valuestr, valuesize) << ' ' << valueno << std::endl;
}

static std::string encodeString( const std::string& value)
{
	const char* hex = "0123456789ABCDEF";
	std::string rt;
	char const* vv = value.c_str();
	for (;*vv; ++vv)
	{
		if ((signed char)*vv <= 32 || *vv == '#')
		{
			rt.push_back('#');
			rt.push_back( hex[ (unsigned char)*vv >>  4]);
			rt.push_back( hex[ (unsigned char)*vv & 0xF]);
		}
		else
		{
			rt.push_back( *vv);
		}
	}
	return rt;
}

ForwardIndexData::ForwardIndexData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	pos = strus::unpackIndex( ki, ke);/*[pos]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of forward index key"));
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
			if (value.size() > 100)
			{
				std::size_t nn = 60;
				while (value.size() > nn && (unsigned char)value[nn] > 127) nn++;
				value.resize( nn);
				value.append( "...");
			}
			std::string encvalue = encodeString( value);
			throw strus::runtime_error( _TXT( "value in forward index is not a valid UTF-8 string: '%s' [%s]"), value.c_str(), encvalue.c_str());
		}
		Index curpos = blk.position_at(bi);
		if (curpos <= prevpos)
		{
			throw strus::runtime_error( _TXT( "positions in forward index are not in strictly ascending order"));
		}
		if (curpos > pos)
		{
			throw strus::runtime_error( _TXT( "position found in forward index that is bigger than the block id"));
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


VariableData::VariableData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	varnamestr = ki;
	varnamesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw strus::runtime_error( _TXT( "illegal UTF8 string as key of global variable"));
	}
	valueno = strus::unpackIndex( vi, ve);/*[value]*/
	if (vi != ve)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of variable value"));
	}
}	

void VariableData::print( std::ostream& out)
{
	out << (char)DatabaseKey::VariablePrefix << ' ' << escapestr( varnamestr, varnamesize) << ' ' << valueno << std::endl;
}


DocMetaDataData::DocMetaDataData( const MetaDataDescription* metadescr, const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	if (ki == ke)
	{
		throw strus::runtime_error( _TXT( "unexpected end of metadata key"));
	}
	blockno = strus::unpackIndex( ki, ke);/*[blockno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of metadata block key"));
	}
	block.init( metadescr, blockno, vi, ve-vi);
	descr = metadescr;
}

void DocMetaDataData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocMetaDataPrefix << ' ' << blockno;
	if (descr->nofElements())
	{
		unsigned int ii = 0;
		for (; ii<MetaDataBlock::BlockSize; ++ii)
		{
			out << ' ';
			MetaDataRecord record = block[ ii];
	
			unsigned int colidx = 0, colend = descr->nofElements();
			for (; colidx<colend; ++colidx)
			{
				if (colidx) out << ',';
				NumericVariant value = record.getValue( descr->get( colidx));
				out << value.tostring().c_str();
			}
		}
	}
	out << std::endl;
}


DocAttributeData::DocAttributeData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki == ke)
	{
		throw strus::runtime_error( _TXT( "unexpected end of document attribute key"));
	}
	attribno = strus::unpackIndex( ki, ke); /*[attribno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "extra character at end of document attribute key"));
	}
	valuestr = vi;
	valuesize = ve-vi;
	if (!strus::checkStringUtf8( vi, ve-vi))
	{
		throw strus::runtime_error( _TXT( "value in document attribute value is not a valid UTF-8 string"));
	}
}	

void DocAttributeData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocAttributePrefix << ' ' << attribno << ' ' << docno << ' ' << escapestr( valuestr, valuesize) << std::endl;
}


DocFrequencyData::DocFrequencyData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	termno = strus::unpackIndex( ki, ke);/*[valueno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of term document frequency key"));
	}
	df = strus::unpackIndex( vi, ve);/*[df]*/
	if (vi != ve)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of df value"));
	}
}

void DocFrequencyData::print( std::ostream& out)
{
	out << (char)DatabaseKey::DocFrequencyPrefix << ' ' << typeno << ' ' << termno << ' ' << df << std::endl;
}


PosinfoBlockData::PosinfoBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	valueno = strus::unpackIndex( ki, ke);/*[valueno]*/
	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of term index key"));
	}
	PosinfoBlock blk( docno, vi, ve-vi);
	PosinfoBlock::Cursor cursor;
	Index dn = blk.firstDoc( cursor);

	while (dn)
	{
		unsigned int ff = (std::size_t)blk.frequency_at( cursor);
		if (dn > docno)
		{
			throw strus::runtime_error( _TXT( "posinfo element docno bigger than upper bound docno"));
		}
		if (posinfo.size() && dn <= posinfo.back().docno)
		{
			throw strus::runtime_error( _TXT( "elements in posinfo array of docno not not strictly ascending"));
		}
		posinfo.push_back( PosinfoPosting( dn, blk.positions_at( cursor)));
		if (ff != posinfo.back().pos.size() && posinfo.back().pos.size() != 0)
		{
			throw strus::runtime_error( _TXT( "ff does not match to length of position array"));
		}
		std::vector<Index>::const_iterator pi = posinfo.back().pos.begin(), pe = posinfo.back().pos.end();
		for (Index prevpos=0; pi != pe; ++pi)
		{
			if (prevpos >= *pi)
			{
				throw strus::runtime_error( _TXT( "position elements in posinfo array not strictly ascending"));
			}
		}
		dn = blk.nextDoc( cursor);
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
	BooleanBlock blk( id, vi, ve-vi);
	BooleanBlock::NodeCursor cursor;

	Index rangemin;
	Index rangemax;
	Index prevmax = 0;

	bool more = blk.getFirstRange( cursor, rangemin, rangemax);
	for (; more; more = blk.getNextRange( cursor, rangemin, rangemax))
	{
		if (rangemax <= 0)
		{
			throw strus::runtime_error( _TXT( "illegal range in boolean block (negative or zero maximum)"));
		}
		if (rangemin <= 0)
		{
			throw strus::runtime_error( _TXT( "illegal range in boolean block (negative or zero minimum)"));
		}
		if (rangemin > rangemax)
		{
			throw strus::runtime_error( _TXT( "illegal range in boolean block (min > max)"));
		}
		if (rangemax > id)
		{
			throw strus::runtime_error( _TXT( "illegal range in boolean block (max > blockId)"));
		}
		if (rangemin <= prevmax)
		{
			throw strus::runtime_error( _TXT( "illegal range in boolean block (not strictly ascending or unjoined overlapping ranges)"));
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

DocListBlockData::DocListBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	typeno = strus::unpackIndex( ki, ke);/*[typeno]*/
	valueno = strus::unpackIndex( ki, ke);/*[valueno]*/
	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of term index key"));
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

InverseTermData::InverseTermData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of inverse term index key"));
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
	out << (char)DatabaseKey::InverseTermPrefix << ' ' << docno;
	std::vector<InvTerm>::const_iterator ti = terms.begin(), te = terms.end();
	for (; ti != te; ++ti)
	{
		out << ' ' << ti->typeno << ',' << ti->termno << ',' << ti->ff;
	}
	out << std::endl;
}


UserAclBlockData::UserAclBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	userno = strus::unpackIndex( ki, ke);/*[userno]*/
	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of user index key"));
	}
	docrangelist = getRangeListFromBooleanBlock( DatabaseKey::UserAclBlockPrefix, docno, vi, ve);
}

void UserAclBlockData::print( std::ostream& out)
{
	out << (char)DatabaseKey::UserAclBlockPrefix << ' ' << userno;
	printRangeList( out, docrangelist);
	out << std::endl;
}


AclBlockData::AclBlockData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	docno = strus::unpackIndex( ki, ke);/*[docno]*/
	userno = strus::unpackIndex( ki, ke);/*[userno]*/
	if (ki != ke)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of docno index key"));
	}
	userrangelist = getRangeListFromBooleanBlock( DatabaseKey::AclBlockPrefix, userno, vi, ve);
}

void AclBlockData::print( std::ostream& out)
{
	out << (char)DatabaseKey::AclBlockPrefix << ' ' << docno;
	printRangeList( out, userrangelist);
	out << std::endl;
}


AttributeKeyData::AttributeKeyData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
{
	char const* ki = key.ptr()+1;
	char const* ke = key.ptr()+key.size();
	char const* vi = value.ptr();
	char const* ve = value.ptr()+value.size();

	varnamestr = ki;
	varnamesize = ke-ki;
	if (!strus::checkStringUtf8( ki, ke-ki))
	{
		throw strus::runtime_error( _TXT( "illegal UTF8 string as attribute key"));
	}
	valueno = strus::unpackIndex( vi, ve);/*[value]*/
	if (vi != ve)
	{
		throw strus::runtime_error( _TXT( "unexpected extra bytes at end of attribute number"));
	}
}

void AttributeKeyData::print( std::ostream& out)
{
	out << (char)DatabaseKey::AttributeKeyPrefix << ' ' << escapestr( varnamestr, varnamesize) << ' ' << valueno << std::endl;
}


MetaDataDescrData::MetaDataDescrData( const strus::DatabaseCursorInterface::Slice& key, const strus::DatabaseCursorInterface::Slice& value)
	:descr( value.ptr())
{
	if (key.size() != 1) std::runtime_error( "illegal (not empty) key for meta data description");
}


void MetaDataDescrData::print( std::ostream& out)
{
	out << (char)DatabaseKey::MetaDataDescrPrefix;
	MetaDataDescription::const_iterator di = descr.begin(), de = descr.end();
	for (int didx=0; di != de; ++di,++didx)
	{
		if (didx) out << ',';
		out << ' ' << di.name() << ' ' << di.element().typeName();
	}
	out << std::endl;
}

