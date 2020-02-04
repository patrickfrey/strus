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
		const std::string& wordType,
		const std::string& texttype,
		const std::string& entitytype,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_),m_textiter(),m_entityiter(),m_typeiter(),m_errorhnd(errorhnd_)
{
	if (!wordType.empty())
	{
		m_typeiter.reset( m_storage->createForwardIterator( wordType));
		if (!m_typeiter.get()) throw std::runtime_error( m_errorhnd->fetchError());
	}
	if (!texttype.empty())
	{
		m_textiter.reset( m_storage->createForwardIterator( texttype));
		if (!m_textiter.get()) throw std::runtime_error( m_errorhnd->fetchError());
	}
	else
	{
		throw strus::runtime_error(_TXT("no type for text to collect defined in '%s'"), THIS_CONTEXT);
	}
	if (!entitytype.empty())
	{
		m_entityiter.reset( m_storage->createForwardIterator( entitytype));
		if (!m_entityiter.get()) throw std::runtime_error( m_errorhnd->fetchError());
	}
}

void ForwardIndexTextCollector::skipDoc( strus::Index docno)
{
	if (m_typeiter.get()) m_typeiter->skipDoc( docno);
	m_textiter->skipDoc( docno);
	if (m_entityiter.get()) m_entityiter->skipDoc( docno);
}

static const char* g_openEntityBracket = "(";
static const char* g_closeEntityBracket = "(";

std::string ForwardIndexTextCollector::fetch( const strus::IndexRange& field)
{
	std::string rt;
	strus::Index pos = m_textiter->skipPos( field.start());
	strus::Index endpos = field.defined() ? field.end() : std::numeric_limits<strus::Index>::max();
	for (; pos && pos < endpos; pos = m_textiter->skipPos( pos+1))
	{
		if (m_typeiter.get() && m_entityiter.get())
		{
			if (pos == m_typeiter->skipPos( pos) && pos == m_entityiter->skipPos( pos))
			{
				strus::Index nextpos = m_typeiter->skipPos( pos+1);
				if (!nextpos) nextpos = pos+1;
				strus::Index pi = pos;
				std::string textstr;
				while (pi < nextpos)
				{
					if (!textstr.empty()) textstr.push_back(' ');
					textstr.append( m_textiter->fetch());
					pi = m_textiter->skipPos( pi+1);
				}
				std::string entitystr = m_entityiter->fetch();
				if (entitystr != textstr)
				{
					if (!rt.empty()) rt.push_back(' ');
					rt.append( g_openEntityBracket);
					rt.append( entitystr);
					rt.append( g_closeEntityBracket);
				}
				pos = nextpos-1;
			}
		}
		else
		{
			if (!rt.empty() && rt.back() != ' ') rt.push_back(' ');
			rt.append( m_textiter->fetch());
		}
		break;
	}
	return rt;
}


