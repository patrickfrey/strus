/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "metaDataElement.hpp"
#include "private/internationalization.hpp"
#include "strus/base/string_conv.hpp"
#include <stdexcept>

using namespace strus;

MetaDataElement::Type MetaDataElement::typeFromName( const char* namestr)
{
	unsigned int ti = 0, te = NofTypes;
	for (; ti<te; ++ti)
	{
		if (strus::caseInsensitiveEquals( namestr, typeName( (Type)ti)))
		{
			return (Type)ti;
		}
	}
	throw strus::runtime_error( _TXT( "unknown meta data element type name '%s'"), namestr);
}


