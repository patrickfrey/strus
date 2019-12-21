/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_BLOCK_STATISTICS_HPP_INCLUDED
#define _STRUS_BLOCK_STATISTICS_HPP_INCLUDED
#include "strus/base/stdint.h"
#include <vector>

namespace strus {

/// \brief Global space usage statistics per block
class BlockStatistics
{
public:
	class Element
	{
	public:
		const std::string& type() const		{return m_type;}
		int64_t size() const			{return m_size;}

		explicit Element( const std::string& type_, int64_t size_=0)
			:m_type(type_),m_size(size_){}
		Element( const Element& o)
			:m_type(o.m_type),m_size(o.m_size){}

	private:
		std::string m_type;
		int64_t m_size;
	};

	/// \brief Constructor
	BlockStatistics()
		:m_elements(){}
	/// \brief Constructor
	explicit BlockStatistics( const std::vector<Element>& elements_)
		:m_elements(elements_){}
	/// \brief Copy constructor
	BlockStatistics( const BlockStatistics& o)
		:m_elements(o.m_elements){}

	/// \brief Get the block statistic elements
	/// \return the block statistic elements
	const std::vector<Element>& elements() const			{return m_elements;}

private:
	std::vector<Element> m_elements;
};

}//namespace
#endif


