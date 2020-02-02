/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_FORWARD_INDEX_COLLECTOR_HPP_INCLUDED
#define _STRUS_FORWARD_INDEX_COLLECTOR_HPP_INCLUDED
#include "strus/forwardIteratorInterface.hpp"
#include "strus/reference.hpp"
#include "strus/index.hpp"
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class StorageClientInterface;

/// \brief Class used to collect items from the forward index
class ForwardIndexCollector
{
public:
	ForwardIndexCollector( StorageClientInterface* storage_, char separator_)
		:m_storage(storage_),m_valueiterar(),m_typeiter()
		,m_lastfield(),m_separator(separator_){}
	ForwardIndexCollector( const ForwardIndexCollector& o)
		:m_storage(o.m_storage),m_valueiterar(o.m_valueiterar),m_typeiter(o.m_typeiter)
		,m_lastfield(o.m_lastfield),m_separator(o.m_separator){}

	void defineTypeFeatureType( const std::string& type);

	/// \brief Add forward iterator for value prioritized in order of definition (first defined used first)
	void addValueFeatureType( const std::string& type);

	/// \brief Get the value from position pos or an empty string if not defined or overlapped by previously fetched item
	std::string fetch( strus::Index pos);

private:
	StorageClientInterface* m_storage;
	typedef strus::Reference<ForwardIteratorInterface> ForwardIteratorRef;
	std::vector<ForwardIteratorRef> m_valueiterar;
	ForwardIteratorRef m_typeiter;
	strus::IndexRange m_lastfield;
	char m_separator;
};

}//namespace
#endif

