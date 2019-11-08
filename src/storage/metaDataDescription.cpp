/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "metaDataDescription.hpp"
#include "private/internationalization.hpp"
#include "strus/databaseClientInterface.hpp"
#include "strus/base/string_conv.hpp"
#include "databaseAdapter.hpp"
#include <cstring>

using namespace strus;

MetaDataDescription::MetaDataDescription()
	:m_bytesize(0)
{}

MetaDataDescription::MetaDataDescription( const DatabaseClientInterface* database_)
	:m_bytesize(0)
{
	load( database_);
}

MetaDataDescription::MetaDataDescription( const MetaDataDescription& o)
	:m_bytesize(o.m_bytesize),m_ar(o.m_ar),m_namemap(o.m_namemap)
{}

static void skipSpaces( char const*& si, const char* se)
{
	for (; si != se
		&& (*si == ' ' || *si == '\t' || *si == '\n' || *si == '\r');
		++si){}
}

static bool isAlnumIdentifier( const std::string& str)
{
	if (str.empty()) return false;
	std::string::const_iterator si = str.begin(), se = str.end();
	for (; si != se; ++si)
	{
		if ((*si|32) >= 'a' && (*si|32) <= 'z') continue;
		if (*si >= '0' && *si <= '9') continue;
		if (*si == '_') continue;

		return false;
	}
	return true;
}

MetaDataDescription::MetaDataDescription( const std::string& str)
	:m_bytesize(0)
{
	char const* si = str.c_str();
	const char* se = str.c_str() + str.size();
	while (si != se)
	{
		skipSpaces( si, se);
		const char* sn = (const char*)std::memchr( si, ' ', se-si);
		if (!sn) throw std::runtime_error( _TXT( "invalid meta data description string"));
		std::string varName( si, sn-si);
		si = sn;
		skipSpaces( si, se);
		sn = (const char*)std::memchr( si, ',', se-si);
		std::string typeName;
		if (!sn)
		{
			typeName = string_conv::trim( std::string( si, se-si));
			si = se;
		}
		else
		{
			typeName = string_conv::trim( std::string( si, sn-si));
			si = sn + 1;
			skipSpaces( si, se);
		}
		add( MetaDataElement::typeFromName( typeName.c_str()), varName);
	}
}

MetaDataDescription& MetaDataDescription::operator=( const MetaDataDescription& o)
{
	m_bytesize = o.m_bytesize;
	m_ar = o.m_ar;
	m_namemap = o.m_namemap;
	return *this;
}

bool MetaDataDescription::isequal( const MetaDataDescription& o) const
{
	return (nofElements() == o.nofElements()
		&& bytesize() == o.bytesize()
		&& m_ar == o.m_ar && m_namemap == o.m_namemap);
}

std::string MetaDataDescription::tostring() const
{
	std::string rt;
	std::vector<MetaDataElement>::const_iterator ei = m_ar.begin(), ee = m_ar.end();
	for (std::size_t eidx=0; ei != ee; ++ei,++eidx)
	{
		if (eidx) rt.push_back( (char)',');

		std::map<std::string,std::size_t>::const_iterator
			ni = m_namemap.begin(), ne = m_namemap.end();
		for (; ni != ne; ++ni)
		{
			if (ni->second == eidx)
			{
				rt.append( ni->first);
				rt.push_back(' ');
				break;
			}
		}
		if (ni == ne) throw strus::runtime_error( _TXT( "corrupt meta data description"));
		rt.append( ei->typeName());
	}
	return rt;
}

int MetaDataDescription::getHandle( const std::string& name_) const
{
	std::map<std::string,std::size_t>::const_iterator
		ni = m_namemap.find( string_conv::tolower( name_));
	if (ni == m_namemap.end()) return -1;
	return (int)ni->second;
}

bool MetaDataDescription::hasElement( const std::string& name_) const
{
	return m_namemap.find( string_conv::tolower( name_)) != m_namemap.end();
}

bool MetaDataDescription::defined( const std::string& name_)
{
	return m_namemap.find( string_conv::tolower( name_)) != m_namemap.end();
}

void MetaDataDescription::add( MetaDataElement::Type type_, const std::string& name_)
{
	if (!isAlnumIdentifier( name_))
	{
		throw strus::runtime_error( _TXT( "meta data name '%s' is not an alphanumeric string"), name_.c_str());
	}
	if (defined( name_))
	{
		throw strus::runtime_error( _TXT( "duplicate definition of meta data element '%s'"), name_.c_str());
	}
	std::size_t ofs = 0;
	std::vector<MetaDataElement>::iterator ei = m_ar.begin(), ee = m_ar.end();
	std::size_t eidx=0;
	for (; ei != ee; ofs += ei->size(), ++ei,++eidx)
	{
		if (ei->size() < MetaDataElement::size( type_))
		{
			break;
		}
	}
	std::map<std::string,std::size_t>::iterator
		ni = m_namemap.begin(), ne = m_namemap.end();
	for (; ni != ne; ++ni)
	{
		if (ni->second >= eidx)
		{
			m_ar[ ni->second].m_ofs += MetaDataElement::size( type_);
			++ni->second;
		}
	}
	m_namemap[ string_conv::tolower( name_)] = eidx;
	m_bytesize += MetaDataElement::size( type_);
	m_ar.insert( ei, MetaDataElement( type_, ofs));
}

void MetaDataDescription::load( const DatabaseClientInterface* database)
{
	std::string descr;
	if (!DatabaseAdapter_MetaDataDescr::load( database, descr))
	{
		*this = MetaDataDescription();
	}
	else
	{
		*this = MetaDataDescription( descr);
	}
}

void MetaDataDescription::store( DatabaseTransactionInterface* transaction)
{
	DatabaseAdapter_MetaDataDescr::store( transaction, tostring());
}

void MetaDataDescription::storeImm( DatabaseClientInterface* database)
{
	DatabaseAdapter_MetaDataDescr::storeImm( database, tostring());
}

MetaDataDescription::TranslationMap
	MetaDataDescription::getTranslationMap(
		const MetaDataDescription& o,
		const std::vector<std::string>& resets) const
{
	TranslationMap rt;
	std::map<std::string,std::size_t>::const_iterator
		ni = o.m_namemap.begin(), ne = o.m_namemap.end();
	for (; ni != ne; ++ni)
	{
		std::map<std::string,std::size_t>::const_iterator
			ti = m_namemap.find( ni->first);
		if (ti != m_namemap.end())
		{
			std::vector<std::string>::const_iterator
				ri = resets.begin(), re = resets.end();
			for (; ri != re && !strus::caseInsensitiveEquals( *ri, ti->first); ++ri){}
			if (ri == re)
			{
				rt.push_back( TranslationElement( &m_ar[ ti->second], &o.m_ar[ ni->second]));
			}
		}
	}
	return rt;
}

std::vector<std::string> MetaDataDescription::columns() const
{
	std::vector<std::string> rt;
	std::size_t cidx=0,cend=m_ar.size();
	for (;cidx < cend; ++cidx)
	{
		std::map<std::string,std::size_t>::const_iterator
			ni = m_namemap.begin(), ne = m_namemap.end();
		for (; ni != ne; ++ni)
		{
			if (ni->second == cidx)
			{
				rt.push_back( ni->first);
				break;
			}
		}
	}
	return rt;
}

void MetaDataDescription::renameElement( const std::string& oldname, const std::string& newname)
{
	std::map<std::string,std::size_t> newnamemap;
	std::map<std::string,std::size_t>::iterator
		ni = m_namemap.begin(), ne = m_namemap.end();
	for (; ni != ne; ++ni)
	{
		if (strus::caseInsensitiveEquals( oldname, ni->first))
		{
			newnamemap[ string_conv::tolower( newname)] = ni->second;
		}
		else
		{
			newnamemap[ ni->first] = ni->second;
		}
	}
	m_namemap = newnamemap;
}

void MetaDataDescription::clear()
{
	m_bytesize = 0;
	m_ar.clear();
	m_namemap.clear();
}


