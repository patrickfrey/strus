/*
 * Copyright (c) 2020 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "forwardIndexCollector.hpp"
#include "private/internationalization.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include <limits>

using namespace strus;

#define THIS_CONTEXT const_cast<char*>("forward index collector")

ForwardIndexCollector::ForwardIndexCollector(
		const StorageClientInterface* storage_,
		char tagSeparator_, const std::string& tagtype,
		ErrorBufferInterface* errorhnd_)
	:m_storage(storage_),m_valueiterar(),m_tagtypeiter()
	,m_tagSeparator(tagSeparator_)
	,m_errorhnd(errorhnd_)
{
	if (!tagtype.empty())
	{
		m_tagtypeiter.reset( m_storage->createForwardIterator( tagtype));
		if (!m_tagtypeiter.get()) throw std::runtime_error( m_errorhnd->fetchError());
	}
}

void ForwardIndexCollector::addFeatureType( const std::string& type)
{
	m_valueiterar.push_back( m_storage->createForwardIterator( type));
	if (!m_valueiterar.back().get()) throw std::runtime_error( m_errorhnd->fetchError());
}

void ForwardIndexCollector::skipDoc( strus::Index docno)
{
	std::vector<ForwardIteratorRef>::iterator vi = m_valueiterar.begin(), ve = m_valueiterar.end();
	for (; vi != ve; ++vi)
	{
		(*vi)->skipDoc( docno);
	}
}

strus::Index ForwardIndexCollector::skipPos( strus::Index pos)
{
	strus::Index rt = 0;
	std::vector<ForwardIteratorRef>::iterator vi = m_valueiterar.begin(), ve = m_valueiterar.end();
	for (; vi != ve; ++vi)
	{
		strus::Index pi = (*vi)->skipPos( pos);
		while (pi && (!rt || pi < rt))
		{
			if (m_tagtypeiter.get() && m_tagtypeiter->skipPos( pi) != pi)
			{
				pi = (*vi)->skipPos( pi+1);
				continue;
			}
			rt = pi;
			break;
		}
	}
	return rt;
}

std::string ForwardIndexCollector::fetch( strus::Index pos)
{
	std::string rt;
	std::vector<ForwardIteratorRef>::iterator vi = m_valueiterar.begin(), ve = m_valueiterar.end();
	for (; vi != ve; ++vi)
	{
		if ((*vi)->skipPos( pos) == pos)
		{
			if (m_tagtypeiter.get())
			{
				if (m_tagtypeiter->skipPos( pos) == pos)
				{
					rt = m_tagtypeiter->fetch();
					if (m_tagSeparator) rt.push_back( m_tagSeparator);
					rt.append( (*vi)->fetch());
				}
			}
			else
			{
				rt.append( (*vi)->fetch());
			}
			break;
		}
	}
	return rt;
}


