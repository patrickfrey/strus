/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions for building field inclusion dependency trees
/// \file fieldtrees.hpp
#ifndef _STRUS_STORAGE_FIELDTREES_LIB_HPP_INCLUDED
#define _STRUS_STORAGE_FIELDTREES_LIB_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include <utility>
#include <vector>
#include <list>

/// \brief strus toplevel namespace
namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;

struct FieldTree
{
	strus::IndexRange range;
	std::list<FieldTree> chld;

	FieldTree( const strus::IndexRange& range_)
		:range(range_),chld(){}
	FieldTree( const FieldTree& o)
		:range(o.range),chld(o.chld){}

	void add( const FieldTree& nd)
		{chld.push_back(nd);}
	void swap( FieldTree& nd)
	{
		chld.swap( nd.chld);
		std::swap( range, nd.range);
	}
	typedef std::list<FieldTree>::const_iterator const_iterator;

	const_iterator begin() const	{return chld.begin();}
	const_iterator end() const	{return chld.end();}
};

/// \brief Takes a list of dependencies and try to build a list of trees where for each tree a parent field (range) covers every child completely, separating overlaps without coverage into a rest list of fields
/// \param[out] rest rest fields that could not be used because they overlap a field without covering it completely
/// \param[in] fieldlist list of fields to build into the result forest
/// \param[in] errorhnd buffer for reporting errors
/// \return list of trees (forest) with the described properties
std::vector<FieldTree> buildFieldTrees(
		std::vector<strus::IndexRange>& rest,
		const std::vector<strus::IndexRange>& fieldlist,
		ErrorBufferInterface* errorhnd);

}//namespace
#endif


