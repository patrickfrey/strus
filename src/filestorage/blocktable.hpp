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
#ifndef _STRUS_KCF_BLOCK_TABLE_HPP_INCLUDED
#define _STRUS_KCF_BLOCK_TABLE_HPP_INCLUDED
#include "strus/position.hpp"
#include "file.hpp"
#include <string>

namespace strus
{

///\class BlockTable
///\brief Implementation of a block map in a file
class BlockTable
{
public:
	BlockTable( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t blocksize_);
	~BlockTable();

	static void create( const std::string& type_, const std::string& name_, const std::string& path_, std::size_t blocksize_);
	void create();
	void open( bool writemode_);
	void close();
	void reset();

	void readBlock( Index idx, void* buf) const;
	void writeBlock( Index idx, const void* buf);
	void partialReadBlock( Index idx, std::size_t pos, void* buf, std::size_t bufsize) const;
	void partialWriteBlock( Index idx, std::size_t pos, const void* buf, std::size_t bufsize);
	Index insertBlock( const void* buf);
	Index fill( std::size_t nof_elements, const void* fillerbuf);
	Index size() const;

private:
	struct ControlBlock
	{
		Index freelist;
	};

	void readControlBlock( Index idx, ControlBlock& block);
	void writeControlBlock( Index idx, const ControlBlock& block);
	Index appendBlock( const void* buf);

private:
	ControlBlock m_controlblock;
	bool m_writemode;
	std::size_t m_blocksize;
	File m_file;
};

} //namespace
#endif


