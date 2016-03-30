/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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

