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
#include "strus/lib/error.hpp"
#include "strus/lib/database_leveldb.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/private/fileio.hpp"
#include "strus/versionStorage.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/databaseInterface.hpp"
#include "private/errorUtils.hpp"
#include "private/internationalization.hpp"
#include "private/utils.hpp"
#include "databaseAdapter.hpp"
#include "storageClient.hpp"
#include "forwardIndexBlock.hpp"
#include "forwardIndexMap.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <memory>

#undef STRUS_LOWLEVEL_DEBUG

static void printUsage()
{
	std::cout << "strusResizeBlocks [options] <config> <blocktype> <newsize>" << std::endl;
	std::cout << "options:" << std::endl;
	std::cout << "-h|--help" << std::endl;
	std::cout << "    " << _TXT("Print this usage and do nothing else") << std::endl;
	std::cout << "-v|--version" << std::endl;
	std::cout << "    " << _TXT("Print the program version and do nothing else") << std::endl;
	std::cout << "    " << _TXT("Write logs to file <FILE>") << std::endl;
	std::cout << "-c|--commit <N>" << std::endl;
	std::cout << "    " << _TXT("Set <N> as number of documents inserted per transaction (default 1000)") << std::endl;
	std::cout << "<config>     : " << _TXT("configuration string of the key/value store database") << std::endl;
	std::cout << "<blocktype>  : " << _TXT("storage block type. One of the following:") << std::endl;
	std::cout << "               forwardindex:" << _TXT("forward index block type") << std::endl;
	std::cout << "<newsize>    : " << _TXT("new size of the blocks, unit depends on block type.") << std::endl;
}

static strus::ErrorBufferInterface* g_errorBuffer = 0;	// error buffer

static unsigned int parseNumber( const char* arg, const char* location)
{
	unsigned int rt = 0;
	char const* ai = arg;
	for (; *ai >= '0' && *ai <= '9'; ++ai)
	{
		rt = rt * 10 + (*ai - '0');
	}
	if (*ai) throw strus::runtime_error( _TXT("number expected as %s"), location);
	return rt;
}

static void commitTransaction( strus::StorageClient& storage, std::auto_ptr<strus::StorageTransactionInterface>& transaction)
{
	if (g_errorBuffer->hasError())
	{
		throw strus::runtime_error(_TXT("error in storage transaction"));
	}
	transaction->commit();
	if (g_errorBuffer->hasError())
	{
		throw strus::runtime_error(_TXT("error in storage transaction commit"));
	}
	transaction.reset( storage.createTransaction());
	if (g_errorBuffer->hasError())
	{
		throw strus::runtime_error(_TXT("error creating new storage transaction"));
	}
}

static void resizeBlocks( strus::DatabaseClientInterface* dbc, const std::string& blocktype, unsigned int newsize, unsigned int transactionsize)
{
	strus::StorageClient storage( dbc, 0/*termnomap_source*/, 0/*statisticsProc*/, g_errorBuffer);
	std::auto_ptr<strus::StorageTransactionInterface> transaction( storage.createTransaction());
	unsigned int transactionidx = 0;
	unsigned int blockcount = 0;
	if (!transaction.get())
	{
		throw strus::runtime_error(_TXT("failed to create storage transaction"));
	}
	if (strus::utils::caseInsensitiveEquals( blocktype, "forwardindex"))
	{
		strus::ForwardIndexMap fwdmap( dbc, storage.maxTermTypeNo(), newsize?newsize:strus::ForwardIndexBlock::MaxBlockTokens);
		strus::Index di = 1, de = storage.maxDocumentNumber()+1;
		for (; di<de; ++di)
		{
			fwdmap.deleteIndex( di);
			fwdmap.openForwardIndexDocument( di);

			strus::Index ti=1, te = storage.maxTermTypeNo()+1;
			for (; ti<te; ++ti)
			{
				strus::ForwardIndexBlock blk;
				strus::DatabaseAdapter_ForwardIndex::Cursor fwdi( dbc, ti, di);
				bool hasmore = fwdi.loadUpperBound( 0, blk);

				for (; hasmore; hasmore=fwdi.loadUpperBound( blk.id()+1, blk))
				{
					char const* blkitr = blk.charptr();
					const char* blkend = blk.charend();
					for (; blkitr < blkend; blkitr = blk.nextItem( blkitr))
					{
						strus::Index pos = blk.position_at( blkitr);
						std::string termstr = blk.value_at( blkitr);
						fwdmap.defineForwardIndexTerm( ti, pos, termstr);
					}
				}
			}
			if (++transactionidx == transactionsize)
			{
				blockcount += transactionidx;
				::printf( "\rresized %u        ", blockcount);
				::fflush( stdout);
				commitTransaction( storage, transaction);
				transactionidx = 0;
			}
		}
		if (transactionidx)
		{
			blockcount += transactionidx;
			::printf( "\rresized %u        ", blockcount);
			::fflush( stdout);
			commitTransaction( storage, transaction);
		}
	}
	else
	{
		throw strus::runtime_error(_TXT("block resize is not implemented for blocktype '%s'"));
	}
}

int main( int argc, const char* argv[])
{
	std::auto_ptr<strus::ErrorBufferInterface> errorBuffer( strus::createErrorBuffer_standard( 0, 2));
	if (!errorBuffer.get())
	{
		std::cerr << _TXT("failed to create error buffer") << std::endl;
		return -1;
	}
	g_errorBuffer = errorBuffer.get();

	try
	{
		bool doExit = false;
		int argi = 1;
		unsigned int transactionsize = 1000;

		// Parsing arguments:
		for (; argi < argc; ++argi)
		{
			if (0==std::strcmp( argv[argi], "-h") || 0==std::strcmp( argv[argi], "--help"))
			{
				printUsage();
				doExit = true;
			}
			else if (0==std::strcmp( argv[argi], "-v") || 0==std::strcmp( argv[argi], "--version"))
			{
				std::cerr << "strusRpc version " << STRUS_STORAGE_VERSION_STRING << std::endl;
				doExit = true;
			}
			else if (0==std::strcmp( argv[argi], "-c") || 0==std::strcmp( argv[argi], "--commit"))
			{
				if (argi == argc || argv[argi+1][0] == '-')
				{
					throw strus::runtime_error( _TXT("no argument given to option --commit"));
				}
				++argi;
				transactionsize = parseNumber( argv[argi], "argument for option --commit");
			}
			else if (argv[argi][0] == '-')
			{
				throw strus::runtime_error(_TXT("unknown option %s"), argv[argi]);
			}
			else
			{
				break;
			}
		}
		if (doExit) return 0;
		if (argc - argi < 3) throw strus::runtime_error( _TXT("too few arguments (given %u, required %u)"), argc - argi, 3);
		if (argc - argi > 3) throw strus::runtime_error( _TXT("too many arguments (given %u, required %u)"), argc - argi, 3);

		std::string dbconfig( argv[ argi+0]);
		std::string blocktype( argv[ argi+1]);
		unsigned int newblocksize = parseNumber( argv[argi+2], "positional argument 3");

		strus::DatabaseInterface* dbi = strus::createDatabase_leveldb( g_errorBuffer);
		if (!dbi) throw strus::runtime_error( _TXT("could not create leveldb key/value store database handler"));
		strus::DatabaseClientInterface* dbc = dbi->createClient( dbconfig);
		if (!dbc) throw strus::runtime_error( _TXT("could not open leveldb key/value store database"));
		if (g_errorBuffer->hasError()) throw strus::runtime_error(_TXT("error in initialization"));

		resizeBlocks( dbc, blocktype, newblocksize, transactionsize);

		// Check for reported error an terminate regularly:
		if (g_errorBuffer->hasError())
		{
			throw strus::runtime_error( _TXT("error processing resize blocks"));
		}
		std::cerr << _TXT("done") << std::endl;
		return 0;
	}
	catch (const std::exception& e)
	{
		const char* errormsg = g_errorBuffer?g_errorBuffer->fetchError():0;
		if (errormsg)
		{
			std::cerr << e.what() << ": " << errormsg << std::endl;
		}
		else
		{
			std::cerr << e.what() << std::endl;
		}
	}
	std::cerr << _TXT("terminated") << std::endl;
	return -1;
}

