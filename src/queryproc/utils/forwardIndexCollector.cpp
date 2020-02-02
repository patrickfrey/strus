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

using namespace strus;

#define CONTEXT "forward index collector"

void ForwardIndexCollector::defineTypeFeatureType( const std::string& type)
{
	if (m_typeiter.get()) throw strus::runtime_error(_TXT("more than one type feature type iterator defined for '%s'"), CONTEXT);
	m_typeiter.reset( m_storage->createForwardIterator( type));
}

void ForwardIndexCollector::addValueFeatureType( const std::string& type)
{
	m_valueiterar.push_back( m_storage->createForwardIterator( type));
}

std::string ForwardIndexCollector::fetch( strus::Index pos)
{
	std::string rt;
	if (pos && m_lastfield.contain( pos) && m_lastfield.start() != pos)
	{
		// ... do not return item covered by last item returned
	}
	else
	{
		std::vector<ForwardIteratorRef>::iterator vi = m_valueiterar.begin(), ve = m_valueiterar.end();
		for (; vi != ve; ++vi)
		{
			if ((*vi)->skipPos( pos) == pos)
			{
				if (m_typeiter.get())
				{
					if (m_typeiter->skipPos( pos) == pos)
					{
						rt.append( m_typeiter->fetch());
						if (m_separator) rt.push_back( m_separator);
						rt.append( (*vi)->fetch());

						strus::Index endpos = m_typeiter->skipPos( pos+1);
						m_lastfield.init( pos, endpos ? endpos : (pos+1));
					}
				}
				else
				{
					rt.append( (*vi)->fetch());
					m_lastfield.init( pos, pos+1);
				}
				break;
			}
		}
	}
	return rt;
}


