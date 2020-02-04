/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_FORWARD_INDEX_TEXT_COLLECTOR_HPP_INCLUDED
#define _STRUS_FORWARD_INDEX_TEXT_COLLECTOR_HPP_INCLUDED
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
/// \brief Forward declaration
class ErrorBufferInterface;


/// \brief Class used to collect text passages from the forward index
class ForwardIndexTextCollector
{
public:
	ForwardIndexTextCollector(
			const StorageClientInterface* storage_,
			const std::string& wordType,
			const std::string& texttype,
			const std::string& entitytype,
			ErrorBufferInterface* errorhnd_);
	ForwardIndexTextCollector( const ForwardIndexTextCollector& o)
		:m_storage(o.m_storage)
		,m_textiter(o.m_textiter),m_entityiter(o.m_entityiter),m_typeiter(o.m_typeiter)
		,m_errorhnd(o.m_errorhnd){}

	void skipDoc( strus::Index docno);

	/// \brief Get the concatenated text of a field from the forward index
	std::string fetch( const strus::IndexRange& field);

private:
	const StorageClientInterface* m_storage;
	typedef strus::Reference<ForwardIteratorInterface> ForwardIteratorRef;
	ForwardIteratorRef m_textiter;
	ForwardIteratorRef m_entityiter;
	ForwardIteratorRef m_typeiter;
	ErrorBufferInterface* m_errorhnd;
};

}//namespace
#endif

