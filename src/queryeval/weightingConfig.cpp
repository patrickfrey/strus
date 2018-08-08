/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/weightingConfig.hpp"
#include "strus/base/dll_tags.hpp"
#include "strus/base/string_conv.hpp"
#include "private/internationalization.hpp"

#error WHO USES THIS FILE
using namespace strus;

DLL_PUBLIC void strus::WeightingConfig::defineNumericParameter( const std::string& name_, const NumericVariant& value_)
{
	try
	{
		std::string name = string_conv::tolower( name_);
		if (m_numericParameters.find( name) != m_numericParameters.end())
		{
			throw strus::runtime_error( _TXT( "duplicate definition of weighting function parameter '%s'"), name_.c_str());
		}
		m_numericParameters[ name] = value_;
	}
	catch (...)
	{
		
	}
}

