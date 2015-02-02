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
#include "metaDataDescription.hpp"
#include "strus/databaseInterface.hpp"
#include "databaseAdapter.hpp"
#include <cstring>
#include <boost/algorithm/string.hpp>

using namespace strus;

MetaDataDescription::MetaDataDescription()
	:m_bytesize(0)
{}

MetaDataDescription::MetaDataDescription( DatabaseInterface* database_)
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
		if (!sn) throw std::runtime_error( "invalid meta data description string");
		std::string varName( si, sn-si);
		si = sn;
		skipSpaces( si, se);
		sn = (const char*)std::memchr( si, ',', se-si);
		std::string typeName;
		if (!sn)
		{
			typeName = boost::algorithm::trim_copy( std::string( si, se-si));
			si = se;
		}
		else
		{
			typeName = boost::algorithm::trim_copy( std::string( si, sn-si));
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
		if (ni == ne) throw std::logic_error( "corrupt meta data description");
		rt.append( ei->typeName());
	}
	return rt;
}

int MetaDataDescription::getHandle( const std::string& name_) const
{
	std::map<std::string,std::size_t>::const_iterator
		ni = m_namemap.find( boost::algorithm::to_lower_copy( name_));
	if (ni == m_namemap.end())
	{
		throw std::runtime_error( std::string( "meta data element with name '") + name_ + "' is not defined");
	}
	return (int)ni->second;
}

bool MetaDataDescription::hasElement( const std::string& name_) const
{
	return m_namemap.find( boost::algorithm::to_lower_copy( name_)) != m_namemap.end();
}

bool MetaDataDescription::defined( const std::string& name_)
{
	return m_namemap.find( boost::algorithm::to_lower_copy( name_)) != m_namemap.end();
}

void MetaDataDescription::add( MetaDataElement::Type type_, const std::string& name_)
{
	if (!isAlnumIdentifier( name_))
	{
		throw std::runtime_error( std::string( "meta data name '") + name_ + "'' is not an alphanumeric string");
	}
	if (defined( name_))
	{
		throw std::runtime_error( std::string( "duplicate definition of meta data element '") + name_ + "'");
	}
	std::size_t ofs = 0;
	std::vector<MetaDataElement>::iterator ei = m_ar.begin(), ee = m_ar.end();
	std::size_t eidx=0;
	for (; ei != ee; ofs += ei->size(), ++ei,++eidx)
	{
		if (ei->size() > MetaDataElement::size( type_))
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
	m_namemap[ boost::algorithm::to_lower_copy( name_)] = eidx;
	m_bytesize += MetaDataElement::size( type_);
	m_ar.insert( ei, MetaDataElement( type_, ofs));
}

void MetaDataDescription::load( DatabaseInterface* database)
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

void MetaDataDescription::storeImm( DatabaseInterface* database)
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
			for (; ri != re && !boost::algorithm::iequals( *ri, ti->first); ++ri){}
			if (ri == re)
			{
				rt.push_back( TranslationElement(
					&m_ar[ ti->second], &o.m_ar[ ni->second]));
			}
		}
	}
	return rt;
}

std::vector<std::string> MetaDataDescription::columns() const
{
	std::vector<std::string> rt;
	std::map<std::string,std::size_t>::const_iterator
		ni = m_namemap.begin(), ne = m_namemap.end();
	std::size_t cidx=0,cend=m_namemap.size();
	for (;cidx < cend; ++cidx)
	{
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
		if (boost::algorithm::iequals( oldname, ni->first))
		{
			newnamemap[ boost::algorithm::to_lower_copy( newname)] = ni->second;
		}
		else
		{
			newnamemap[ ni->first] = ni->second;
		}
	}
	m_namemap = newnamemap;
}

