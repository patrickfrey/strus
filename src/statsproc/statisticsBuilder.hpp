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
/// \brief Implementation of the interface for building messages to populate some statistics (distributed index)
/// \file statisticsBuilderInterface.hpp
#ifndef _STRUS_STATISTICS_BUILDER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_STATISTICS_BUILDER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/statisticsBuilderInterface.hpp"
#include "compactNodeTrie.hpp"
#include <string>
#include <vector>
#include <list>

namespace strus
{
///\brief Forward declaration
class ErrorBufferInterface;

class StatisticsBuilder
	:public StatisticsBuilderInterface
{
public:
	StatisticsBuilder( bool insertInLexicalOrder_, std::size_t maxblocksize_, ErrorBufferInterface* errorhnd);
	virtual ~StatisticsBuilder();

	virtual void setNofDocumentsInsertedChange(
			int increment);

	virtual void addDfChange(
			const char* termtype,
			const char* termvalue,
			int increment);

	virtual bool fetchMessage( const char*& blk, std::size_t& blksize);

	virtual void start();

	virtual void rollback();

private:
	void addDfChange_final(
			const std::string& key,
			int increment);
	void addDfChange_tree(
			const std::string& key,
			int increment);
	void moveTree();
	void newContent();
	void clear();

private:
	bool m_insertInLexicalOrder;
	std::string m_lastkey;
	std::list<std::string> m_content;
	bool m_content_consumed;
	conotrie::CompactNodeTrie m_tree;
	std::size_t m_cnt;
	std::size_t m_blocksize;
	std::size_t m_maxblocksize;
	ErrorBufferInterface* m_errorhnd;
};
}//namespace
#endif
