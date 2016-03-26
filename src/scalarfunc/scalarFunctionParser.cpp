/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Default implementation of the strus scalar function parser
/// \file scalarFunctionParser.cpp
#include "scalarFunctionParser.hpp"
#include "scalarFunction.hpp"
#include "private/utils.hpp"

using namespace strus;

void ScalarFunctionParser::defineUnaryFunction( const std::string& name, UnaryFunction func)
{
	try
	{
		m_unaryFunctionMap[ strus::utils::tolower(name)] = func;
	}
	CATCH_ERROR_MAP( _TXT("error defining unary function (scalar function parser): %s"), *m_errorhnd);
}

void ScalarFunctionParser::defineBinaryFunction( const std::string& name, BinaryFunction func)
{
	try
	{
		m_binaryFunctionMap[ strus::utils::tolower(name)] = func;
	}
	CATCH_ERROR_MAP( _TXT("error defining binary function (scalar function parser): %s"), *m_errorhnd);
}

void ScalarFunctionParser::defineNaryFunction( const std::string& name, NaryFunction func)
{
	try
	{
		m_naryFunctionMap[ strus::utils::tolower(name)] = func;
	}
	CATCH_ERROR_MAP( _TXT("error defining N-ary function (scalar function parser): %s"), *m_errorhnd);
}

ScalarFunctionInterface* ScalarFunctionParser::createFunction( const std::string& src) const
{
	const char* si = src.c_str();
	const char* se = src.c_str() + src.size();
	try
	{
		std::auto_ptr<ScalarFunction> func( new ScalarFunction( m_errorhnd));
	
		for (;si != se; ++si)
		{
			
		}
		ScalarFunction* rt = func.get();
		func.release();
		return rt;
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error executing scalar function: %s"), *m_errorhnd, 0);
}


