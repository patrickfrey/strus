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
#include "strus/storage/index.hpp"
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Class used to collect items from the forward index
class ForwardIndexCollector
{
public:
	ForwardIndexCollector(
			const StorageClientInterface* storage_,
			char tagSeparator_, const std::string& tagtype,
			ErrorBufferInterface* errorhnd_);
	ForwardIndexCollector( const ForwardIndexCollector& o)
		:m_storage(o.m_storage),m_valueiterar(o.m_valueiterar),m_tagtypeiter(o.m_tagtypeiter)
		,m_tagSeparator(o.m_tagSeparator),m_curidx(o.m_curidx)
		,m_errorhnd(o.m_errorhnd){}

	/// \brief Add forward iterator for value prioritized in order of definition (first defined used first)
	void addFeatureType( const std::string& valuetype);

	void skipDoc( strus::Index docno);

	strus::Index skipPos( strus::Index pos);

	/// \brief Get the value from position pos or an empty string if not defined or overlapped by previously fetched item
	std::string fetch();

private:
	const StorageClientInterface* m_storage;
	typedef strus::Reference<ForwardIteratorInterface> ForwardIteratorRef;
	std::vector<ForwardIteratorRef> m_valueiterar;
	ForwardIteratorRef m_tagtypeiter;
	char m_tagSeparator;
	int m_curidx;
	ErrorBufferInterface* m_errorhnd;
};

}//namespace
#endif

