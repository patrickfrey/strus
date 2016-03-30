/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "nullPostingIterator.hpp"
#include "indexPacker.hpp"

using namespace strus;

#undef STRUS_LOWLEVEL_DEBUG

#ifdef STRUS_LOWLEVEL_DEBUG
NullPostingIterator::NullPostingIterator( const char* termstr)
{
	m_featureid.append( termstr);
	m_featureid.append( "!NIL");
}
#else
NullPostingIterator::NullPostingIterator( const char*)
{
	m_featureid.append( "!NIL");
}
#endif
