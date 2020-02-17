/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "forwardIndexTextCollector.hpp"
#include "private/internationalization.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include <limits>

using namespace strus;

#define THIS_CONTEXT const_cast<char*>("forward index text collector")

ForwardIndexTextCollector::ForwardIndexTextCollector(
		const StorageClientInterface* storage_,
		const std::string& textType,
		const std::string& entityType,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_),m_textiter(),m_entityiter()
	,m_errorhnd(errorhnd_)
{
	if (!textType.empty())
	{
		m_textiter.reset( m_storage->createForwardIterator( textType));
		if (!m_textiter.get()) throw std::runtime_error( m_errorhnd->fetchError());
	}
	else
	{
		throw strus::runtime_error(_TXT("no type for text to collect defined in '%s'"), THIS_CONTEXT);
	}
	if (!entityType.empty())
	{
		m_entityiter.reset( m_storage->createForwardIterator( entityType));
		if (!m_entityiter.get()) throw std::runtime_error( m_errorhnd->fetchError());
	}
}

ForwardIndexTextCollector::ForwardIndexTextCollector(
		const StorageClientInterface* storage_,
		const std::string& textType,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_),m_textiter(),m_entityiter(),m_errorhnd(errorhnd_)
{
	if (!textType.empty())
	{
		m_textiter.reset( m_storage->createForwardIterator( textType));
		if (!m_textiter.get()) throw std::runtime_error( m_errorhnd->fetchError());
	}
	else
	{
		throw strus::runtime_error(_TXT("no type for text to collect defined in '%s'"), THIS_CONTEXT);
	}
}

void ForwardIndexTextCollector::skipDoc( strus::Index docno)
{
	m_textiter->skipDoc( docno);
	if (m_entityiter.get()) m_entityiter->skipDoc( docno);
}

static const char* g_openEntityBracket = "[";
static const char* g_closeEntityBracket = "]";

std::string ForwardIndexTextCollector::fetch( const strus::IndexRange& field)
{
	std::string rt;
	strus::Index pos = m_textiter->skipPos( field.start());
	strus::Index endpos = field.defined() ? field.end() : std::numeric_limits<strus::Index>::max();
	for (; pos && pos < endpos; pos = m_textiter->skipPos( pos+1))
	{
		std::string textstr = m_textiter->fetch();
		if (m_entityiter.get())
		{
			strus::Index epos = m_entityiter->skipPos( pos);
			if (pos == epos)
			{
				std::string entitystr = m_entityiter->fetch();
				if (textstr != entitystr)
				{
					if (!rt.empty() && rt[ rt.size()-1] != ' ') rt.push_back(' ');
					rt.append( g_openEntityBracket);
					rt.append( entitystr);
					rt.append( g_closeEntityBracket);
				}
			}
		}
		if (!rt.empty() && rt[ rt.size()-1] != ' ') rt.push_back(' ');
		rt.append( textstr);
	}
	return rt;
}

