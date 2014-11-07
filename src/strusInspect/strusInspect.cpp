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
#include "strus/storageLib.hpp"
#include "strus/iteratorInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/index.hpp"
#include "private/iteratorReference.hpp"
#include "private/storageReference.hpp"
#include "private/forwardIndexViewerReference.hpp"
#include "indexPacker.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>

static strus::Index stringToIndex( const char* value)
{
	std::ostringstream val;
	return boost::lexical_cast<strus::Index>( std::string( value));
}
static bool isIndex( char const* cc)
{
	while (*cc >= '0' && *cc <= '9') ++cc;
	return (*cc == '\0');
}

static void inspectPositions( strus::StorageInterface& storage, const char** key, int size)
{
	if (size > 3) throw std::runtime_error( "too many arguments");
	if (size < 3) throw std::runtime_error( "too few arguments");
	strus::IteratorReference itr(
		storage.createTermOccurrenceIterator(
			std::string(key[0]), std::string(key[1])));

	strus::Index docno = isIndex(key[2])
			?stringToIndex( key[2])
			:storage.documentNumber( key[2]);
	if (docno == itr->skipDoc( docno))
	{
		strus::Index pos=0;
		int cnt = 0;
		while (0!=(pos=itr->skipPos(pos+1)))
		{
			if (cnt++ != 0) std::cout << " ";
			std::cout << pos;
		}
		std::cout << std::endl;
	}
}

static void inspectDocumentFrequency( strus::StorageInterface& storage, const char** key, int size)
{
	if (size > 2) throw std::runtime_error( "too many arguments");
	if (size < 2) throw std::runtime_error( "too few arguments");

	strus::IteratorReference itr(
		storage.createTermOccurrenceIterator(
			std::string(key[0]), std::string(key[1])));
	std::cout << itr->documentFrequency();
}

static void inspectFeatureFrequency( strus::StorageInterface& storage, const char** key, int size)
{
	if (size > 2) throw std::runtime_error( "too many arguments");
	if (size < 2) throw std::runtime_error( "too few arguments");
	strus::IteratorReference itr(
		storage.createTermOccurrenceIterator(
			std::string(key[0]), std::string(key[1])));

	std::cout << (*itr).frequency() << std::endl;
}

static void inspectDocAttribute( const strus::StorageInterface& storage, const char** key, int size)
{
	if (size > 2) throw std::runtime_error( "too many arguments");
	if (size < 2) throw std::runtime_error( "too few arguments");
	if (std::strlen(key[0]) != 1) throw std::runtime_error( "single character for document metadata id expected");

	strus::Index docno = isIndex(key[1])
			?stringToIndex( key[1])
			:storage.documentNumber( key[1]);

	std::cout << storage.documentAttribute( docno, key[0][0]) << std::endl;
}

static void inspectDocMetaData( const strus::StorageInterface& storage, const char** key, int size)
{
	if (size > 2) throw std::runtime_error( "too many arguments");
	if (size < 2) throw std::runtime_error( "too few arguments");
	if (std::strlen(key[0]) != 1) throw std::runtime_error( "single character for document metadata id expected");

	strus::Index docno = isIndex(key[1])
			?stringToIndex( key[1])
			:storage.documentNumber( key[1]);

	boost::scoped_ptr<strus::MetaDataReaderInterface> reader(
		storage.createMetaDataReader( key[0][1]));

	std::cout << reader->readValue( docno) << std::endl;
}

static void inspectContent( strus::StorageInterface& storage, const char** key, int size)
{
	if (size > 2) throw std::runtime_error( "too many arguments");
	if (size < 2) throw std::runtime_error( "too few arguments");

	strus::Index docno = isIndex(key[1])
			?stringToIndex( key[1])
			:storage.documentNumber( key[1]);

	strus::ForwardIndexViewerReference viewer( storage.createForwardIndexViewer( std::string(key[0])));
	viewer->initDoc( docno);
	strus::Index pos=0;
	while (0!=(pos=viewer->skipPos(pos+1)))
	{
		std::cout << viewer->fetch() << ' ';
	}
}

static void inspectToken( strus::StorageInterface& storage, const char** key, int size)
{
	if (size > 2) throw std::runtime_error( "too many arguments");
	if (size < 2) throw std::runtime_error( "too few arguments");

	strus::Index docno = isIndex(key[1])
			?stringToIndex( key[1])
			:storage.documentNumber( key[1]);

	strus::ForwardIndexViewerReference viewer( storage.createForwardIndexViewer( std::string(key[0])));
	viewer->initDoc( docno);
	strus::Index pos=0;
	while (0!=(pos=viewer->skipPos(pos+1)))
	{
		std::cout << "[" << pos << "] " << viewer->fetch() << std::endl;
	}
}



int main( int argc, const char* argv[])
{
	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		std::cerr << "usage: strusInspect <config> <what> <key>" << std::endl;
		std::cerr << "<config>  : configuration string of the storage" << std::endl;
		std::string indent;
		char const* cc = strus::getStorageConfigDescription();
		char const* ee;
		do
		{
			ee = std::strchr( cc,'\n');
			std::string line = ee?std::string( cc, ee-cc):std::string( cc);
			std::cerr << indent << line << std::endl;
			cc = ee + 1;
			if (indent.empty())
			{
				indent = std::string( 12, ' ');
			}
		}
		while (ee);
		std::cerr << "<what>    : identifier of what to inspect" << std::endl;
		std::cerr << "<key>     : key of the value to retrieve" << std::endl;
		return 0;
	}
	try
	{
		if (argc < 3) throw std::runtime_error( "too few arguments (expected storage configuration string)");

		strus::StorageReference storage(
			strus::createStorageClient( argv[1]));
		
		if (0==std::strcmp( argv[2], "pos"))
		{
			inspectPositions( *storage, argv+3, argc-3);
		}
		else if (0==std::strcmp( argv[2], "ff"))
		{
			inspectFeatureFrequency( *storage, argv+3, argc-3);
		}
		else if (0==std::strcmp( argv[2], "df"))
		{
			inspectDocumentFrequency( *storage, argv+3, argc-3);
		}
		else if (0==std::strcmp( argv[2], "meta"))
		{
			inspectDocMetaData( *storage, argv+3, argc-3);
		}
		else if (0==std::strcmp( argv[2], "attrib"))
		{
			inspectDocAttribute( *storage, argv+3, argc-3);
		}
		else if (0==std::strcmp( argv[2], "content"))
		{
			inspectContent( *storage, argv+3, argc-3);
		}
		else if (0==std::strcmp( argv[2], "token"))
		{
			inspectToken( *storage, argv+3, argc-3);
		}
		else
		{
			throw std::runtime_error( std::string( "unknown item name '") + argv[2] + "'");
		}
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << "ERROR " << e.what() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "EXCEPTION " << e.what() << std::endl;
	}
	return -1;
}


