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
#include "strus/fileio.hpp"
#include "strus/queryEvalLib.hpp"
#include "strus/queryEvalInterface.hpp"
#include "strus/queryProcessorLib.hpp"
#include "strus/queryProcessorInterface.hpp"
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <boost/scoped_ptr.hpp>

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " <program>" << std::endl;
	std::cerr << "<program> = file with query evaluation program source" << std::endl;
}

static bool compareText( const std::string& aa, const std::string& bb)
{
	std::string::const_iterator ai = aa.begin(), ae = aa.end();
	std::string::const_iterator bi = bb.begin(), be = bb.end();

	while (ai != ae && bi != be)
	{
		if (*ai == *bi)
		{
			++ai;
			++bi;
			continue;
		}
		else if (*ai == '\r')
		{
			++ai;
		}
		else if (*bi == '\r')
		{
			++bi;
		}
		else
		{
			return false;
		}
	}
	if (ai != ae)
	{
		return false;
	}
	if (bi != be)
	{
		return false;
	}
	return true;
}

int main( int argc, const char* argv[])
{
	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		printUsage( argc, argv);
		return 0;
	}
	else if (argc > 2)
	{
		std::cerr << "ERROR too many parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	try
	{
		const char* prgext = ".prg";
		std::size_t prgextsize = std::strlen( prgext);
		const char* outext = ".res";
		std::size_t outextsize = std::strlen( outext);
		const char* expext = ".exp";
		std::size_t expextsize = std::strlen( expext);

		std::string prgpath = argv[1];
		std::vector<std::string> prgfiles;
		if (!strus::isDir( prgpath))
		{
			if (prgpath.size() < prgextsize)
			{
				std::cerr << "ERROR file with extension '" << prgext << "'' expected" << std::endl;
				return 2;
			}
			const char* ee = prgpath.c_str() + prgpath.size() - prgextsize;
			if (0==std::strcmp( ee, prgext))
			{
				prgfiles.push_back( prgpath);
			}
			else
			{
				std::cerr << "ERROR file with extension '" << prgext << "'' expected" << std::endl;
			}
		}
		else
		{
			std::vector<std::string> prgnames;
			unsigned int ec = strus::readDir( prgpath, prgext, prgnames);
			if (ec)
			{
				std::cerr << "ERROR could not read program directory '" << prgpath << "', error " << ec << std::endl;
				return 3;
			}
			std::vector<std::string>::const_iterator pi = prgnames.begin(), pe = prgnames.end();
			for (; pi != pe; ++pi)
			{
				prgfiles.push_back( prgpath + strus::dirSeparator() + *pi);
			}
		}
		std::vector<std::string>::const_iterator fi = prgfiles.begin(), fe = prgfiles.end();
		for (; fi != fe; ++fi)
		{
			std::cerr << "executing test '" << *fi << "'" << std::endl;
			std::string outfile
					= std::string( fi->c_str(), fi->size() - prgextsize)
					+ std::string( outext, outextsize);
			std::string expfile
					= std::string( fi->c_str(), fi->size() - prgextsize)
					+ std::string( expext, expextsize);

			std::string prgsource;
			unsigned int ec = strus::readFile( *fi, prgsource);
			if (ec)
			{
				std::cerr << "ERROR could not read program file '" << *fi << "', error " << ec << std::endl;
				return 4;
			}
			std::string expstr;
			ec = strus::readFile( expfile, expstr);
			if (ec)
			{
				std::cerr << "ERROR could not read expected result file '" << expfile << "', error " << ec << std::endl;
			}
			boost::scoped_ptr<strus::QueryProcessorInterface> qproc(
				strus::createQueryProcessorInterface( 0));

			boost::scoped_ptr<strus::QueryEvalInterface> qeval(
				strus::createQueryEval( qproc.get(), prgsource));
			std::ostringstream out;
			qeval->print( out);
			std::string outstr( out.str());

			if (!compareText( outstr, expstr))
			{
				ec = strus::writeFile( outfile, outstr);
				if (ec)
				{
					std::cerr << "ERROR could not write test output to file '" << outfile << "', error " << ec << std::endl;
				}
				std::cerr << "ERROR expected output of test '" << *fi << "' did not match as expected" << std::endl;
				return 5;
			}
		}
		return 0;
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


