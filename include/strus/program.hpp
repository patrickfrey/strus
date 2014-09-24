/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#ifndef _STRUS_PROGRAM_HPP_INCLUDED
#define _STRUS_PROGRAM_HPP_INCLUDED
#include "storageInterface.hpp"
#include <string>
#include <vector>
#include <map>

namespace strus
{

class Program
{
public:
	Program();
	Program( const Program& o);

	IteratorInterfaceR getIterator( const StorageInterfaceR& storage);

private:
	struct Command
	{
		enum Type {StorageFetch,Union,Intersect,CutInRange};
		Type m_type;
		int m_operandref[3];

		Command( const Command& o);
		Command( Type type_, int op1=0, int op2=0, int op3=0);
	};

private:
	friend class Program::Result;
	std::vector<std::string> m_strings;
	std::vector<Command> m_cmdlist;
	std::vector<bool> m_refargset;
	std::size_t m_nofrefarg;
};

}//namespace

#endif


