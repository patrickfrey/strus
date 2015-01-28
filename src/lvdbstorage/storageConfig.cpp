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
#include "storageConfig.hpp"
#include "strus/index.hpp"
#include <map>
#include <cstring>
#include <stdexcept>
#include <boost/algorithm/string.hpp>

using namespace strus;

namespace {
class CaseInsensitiveKey
	:public std::string
{
public:
	CaseInsensitiveKey(){}
	CaseInsensitiveKey( const std::string& o)
		:std::string( boost::algorithm::to_lower_copy( o)){}
	CaseInsensitiveKey( const CaseInsensitiveKey& o)
		:std::string( o){}
};

class ConfigMap
	:public std::map<CaseInsensitiveKey,std::string>
{
public:
	ConfigMap( const char* config);
};

ConfigMap::ConfigMap( const char* config)
{
	char const* cc = config;
	while (*cc)
	{
		std::string key;
		while ((*cc|32) >= 'a' && (*cc|32) <= 'z')
		{
			key.push_back( *cc++);
		}
		if (*cc != '=')
		{
			throw std::runtime_error( "'=' expected after item identifier in config string");
		}
		++cc;
		const char* ee = std::strchr( cc, ';');
		if (!ee) ee = std::strchr( cc, '\0');
		(*this)[ key] = std::string( cc,ee-cc);
		cc = (*ee)?(ee+1):ee;
	}
}

static unsigned int nofK( const std::string& numstr)
{
	Index rt = 0;
	char const* cc = numstr.c_str();
	for (;*cc; ++cc)
	{
		if (*cc >= '0' && *cc <= '9')
		{
			rt = (rt * 10) + (*cc - '0');
		}
		else if (*cc == 'K' || *cc == 'k')
		{
			rt = rt * 1024;
			++cc;
			break;
		}
		else if (*cc == 'M' || *cc == 'm')
		{
			rt = rt * 1024 * 1024;
			++cc;
			break;
		}
		else if (*cc == 'G' || *cc == 'g')
		{
			rt = rt * 1024 * 1024 * 1024;
			++cc;
			break;
		}
	}
	if (!cc)
	{
		throw std::runtime_error( std::string( "not a number (with optional 'K' or 'M' or 'G' suffix) for configuration option 'cache': '") + numstr + "'");
	}
	return (unsigned int)((rt + 1023)/1024);
}

static bool yesNo( const char* cfgname, const std::string& str)
{
	std::string lostr = boost::algorithm::to_lower_copy( str);
	if (lostr == "y") return true;
	if (lostr == "n") return false;
	if (lostr == "t") return true;
	if (lostr == "f") return false;
	if (lostr == "yes") return true;
	if (lostr == "no") return false;
	if (lostr == "true") return true;
	if (lostr == "false") return false;
	if (lostr == "1") return true;
	if (lostr == "0") return false;
	throw std::runtime_error( std::string("value for configuration option '") + cfgname + "' is not a boolean (yes/no or true/false)");
}

}//namespace

StorageConfig::StorageConfig( const char* source)
	:m_acl(false),m_cachesize_kb(0),m_compression(true)
{
	bool cache_defined = false;
	bool acl_defined = false;
	bool compression_defined = false;

	ConfigMap configMap( source);
	ConfigMap::const_iterator ci = configMap.begin(), ce = configMap.end();

	for (; ci != ce; ++ci)
	{
		if (ci->first == "path")
		{
			if (m_path.size()) throw std::runtime_error( "duplicate definition of 'path' in storage config");
			m_path = ci->second;
			if (m_path.empty()) throw std::runtime_error( "empty definition of 'path' in storage config");
		}
		else if (ci->first == "metadata")
		{
			if (m_metadata.size()) throw std::runtime_error( "duplicate definition of 'metadata' in storage config");
			m_metadata = ci->second;
			if (m_metadata.empty()) throw std::runtime_error( "empty definition of 'metadata' in storage config");
		}
		else if (ci->first == "cachedterms")
		{
			if (m_cachedterms.size()) throw std::runtime_error( "duplicate definition of 'cachedterms' in storage config");
			m_cachedterms = ci->second;
			if (m_cachedterms.empty()) throw std::runtime_error( "empty definition of 'cachedterms' in storage config");
		}
		else if (ci->first == "acl")
		{
			if (acl_defined) throw std::runtime_error( "duplicate definition of 'acl' in storage config");
			m_acl = yesNo( "acl", ci->second);
			acl_defined = true;
		}
		else if (ci->first == "compression")
		{
			if (compression_defined) throw std::runtime_error( "duplicate definition of 'acl' in storage config");
			m_compression = yesNo( "compression", ci->second);
			compression_defined = true;
		}
		else if (ci->first == "cache")
		{
			if (cache_defined) throw std::runtime_error( "duplicate definition of 'cache' in storage config");
			m_cachesize_kb = nofK( ci->second);
			cache_defined = true;
		}
		else
		{
			throw std::runtime_error( std::string( "unknown configuration option '") + ci->first +"'");
		}
	}
	if (m_path.empty())
	{
		throw std::runtime_error( "no path defined in config for levelDB storage");
	}
}

