/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/private/configParser.hpp"
#include "strus/index.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/dll_tags.hpp"
#include "private/utils.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <map>
#include <cstring>
#include <stdexcept>

using namespace strus;

DLL_PUBLIC bool strus::extractStringFromConfigString( std::string& res, std::string& config, const char* key, ErrorBufferInterface* errorhnd)
{
	try
	{
		char const* cc = config.c_str();
		while (*cc)
		{
			while (*cc && (unsigned char)*cc <= 32) ++cc;
			//... skip spaces
			if (!*cc) break;

			const char* start = cc;
			std::string cfgkey;
			while (((*cc|32) >= 'a' && (*cc|32) <= 'z') || *cc == '_' || (*cc >= '0' && *cc <= '9'))
			{
				cfgkey.push_back( *cc++);
			}
			if (cfgkey.empty())
			{
				throw strus::runtime_error( _TXT( "expected item identifier as start of a declaration in a config string ('%s' | '%s')"), cfgkey.c_str(), config.c_str());
			}
			if (*cc != '=')
			{
				throw strus::runtime_error( _TXT( "'=' expected after item identifier in a config string ('%s %s' | '%s')"), cfgkey.c_str(), cc, config.c_str());
			}
			++cc;
			const char* ee = std::strchr( cc, ';');
			if (!ee) ee = std::strchr( cc, '\0');
			if (utils::caseInsensitiveEquals( cfgkey, key))
			{
				res = std::string( cc, ee - cc);
				std::string rest_config( config.c_str(), start);
				if (*ee) rest_config.append( ee+1);
				config = rest_config;
				return true;
			}
			else
			{
				cc = (*ee)?(ee+1):ee;
			}
		}
		return false;
	}
	CATCH_ERROR_MAP_RETURN("error extracting string from configuration string: %s", *errorhnd, false);
}

static bool yesNoFromString( const char* cfgname, const std::string& str)
{
	std::string lostr = utils::tolower( str);
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
	throw strus::runtime_error( _TXT( "value for configuration option '%s' is not a boolean (yes/no or true/false)"), cfgname);
}

DLL_PUBLIC bool strus::extractBooleanFromConfigString( bool& val, std::string& config, const char* key, ErrorBufferInterface* errorhnd)
{
	try
	{
		std::string cfgval;
		if (extractStringFromConfigString( cfgval, config, key, errorhnd))
		{
			val = yesNoFromString( key, cfgval);
			return true;
		}
		else
		{
			return false;
		}
	}
	CATCH_ERROR_MAP_RETURN("error extracting unsigned integer from configuration string: %s", *errorhnd, false);
}

static unsigned int unsignedFromString( const std::string& numstr)
{
	unsigned int rt = 0;
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
		throw strus::runtime_error( _TXT( "not a number (with optional 'K' or 'M' or 'G' suffix) for configuration option 'cache': '%s'"), numstr.c_str());
	}
	return (unsigned int)((rt + 1023)/1024);
}


DLL_PUBLIC bool strus::extractUIntFromConfigString( unsigned int& val, std::string& config, const char* key, ErrorBufferInterface* errorhnd)
{
	try
	{
		std::string cfgval;
		if (extractStringFromConfigString( cfgval, config, key, errorhnd))
		{
			val = unsignedFromString( cfgval);
			return true;
		}
		else
		{
			return false;
		}
	}
	CATCH_ERROR_MAP_RETURN("error extracting unsigned integer from configuration string: %s", *errorhnd, false);
}


DLL_PUBLIC void strus::removeKeysFromConfigString( std::string& config, const char** keys, ErrorBufferInterface* errorhnd)
{
	try
	{
		std::string val;
		for (std::size_t ii=0; keys[ii]; ++ii)
		{
			extractStringFromConfigString( val, config, keys[ii], errorhnd);
		}
	}
	CATCH_ERROR_MAP("error removing keys from configuration string: %s", *errorhnd);
}


