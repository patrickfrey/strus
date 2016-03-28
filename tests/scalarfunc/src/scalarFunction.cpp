/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "strus/lib/scalarfunc.hpp"
#include "strus/lib/error.hpp"
#include "strus/scalarFunctionInterface.hpp"
#include "strus/scalarFunctionParserInterface.hpp"
#include "strus/scalarFunctionInstanceInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <stdexcept>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <iomanip>

using namespace strus;
#undef STRUS_LOWLEVEL_DEBUG

ErrorBufferInterface* g_errorhnd = 0;

static ScalarFunctionParserInterface* createScalarFunctionParser( ErrorBufferInterface* errorhnd)
{
	std::auto_ptr<ScalarFunctionParserInterface> sfp( createScalarFunctionParser_default( errorhnd));
	if (!sfp.get())
	{
		throw strus::runtime_error( "failed to create parser: %s", errorhnd->fetchError());
	}
	ScalarFunctionParserInterface* rt = sfp.get();
	sfp.release();
	return rt;
}

struct VariableDef
{
	const char* name;
	double value;
};

struct Test
{
	const char* name;
	const char* formula;
	const VariableDef variables[ 10];
	std::size_t nofparameter;
	double parameter[10];
	double result;
};

static bool run( const ScalarFunctionParserInterface* parser, unsigned int testidx, const Test& test)
{
	std::auto_ptr<ScalarFunctionInterface> funcdef( parser->createFunction( test.formula));
	if (!funcdef.get())
	{
		throw strus::runtime_error( "failed to create function for test '%s': %s", test.name, g_errorhnd->fetchError());
	}
	std::auto_ptr<ScalarFunctionInstanceInterface> func( funcdef->createInstance());
	if (!func.get())
	{
		throw strus::runtime_error( "failed to create function instance for test '%s': %s", test.name, g_errorhnd->fetchError());
	}
	const VariableDef* vi = test.variables;
	for (;vi->name; ++vi)
	{
		func->setVariableValue( vi->name, vi->value);
	}
	double result = func->call( test.parameter, test.nofparameter);
	if (g_errorhnd->hasError())
	{
		throw strus::runtime_error( "failed to execute test '%s': %s", test.name, g_errorhnd->fetchError());
	}
	double xx = (result - test.result);
	bool rt = (xx * xx < (std::numeric_limits<double>::epsilon()*10));
	if (!rt)
	{
		std::cerr << "[" << testidx << "] test '" << test.name << "'" << std::endl;
		std::cerr << "function:" << std::endl;
		std::cerr << func->tostring() << std::cerr;

		std::cerr << std::setprecision(11) << "result=" << result << " expected=" << test.result << " diff=" << xx << std::endl;
	}
	return rt;
}

static Test tests[] =
{
	{
		"variable",
		"x1",
		{{"x1",1.2},{0,0.0}},
		0,
		{0.0},
		1.2
	},
	{
		"variable sum",
		"x1 + x3",
		{{"x1",1.2},{"x3",-1.9},{0,0.0}},
		0,
		{0.0},
		-0.7
	},
	{
		"variable argument sum",
		"x + _0",
		{{"x",2.7},{0,0.0}},
		1,
		{0.4},
		3.1
	},
	{
		"argument argument subtraction",
		"_0 - _1",
		{{0,0.0}},
		2,
		{17.8,3.1},
		14.7
	},
	{
		"linear combination",
		"x * _0 - y * _1",
		{{"x",2.2},{"y",1.1},{0,0.0}},
		2,
		{0.5,0.1},
		0.99
	},
	{
		"normalized log",
		"log( x*x - y + _0)",
		{{"x",2.0},{"y",3.0},{0,0.0}},
		1,
		{0.5},
		0.17609125905568124
	},
	{
		"log pow base 10",
		"log( pow( x, 2))",
		{{"x",10.0},{0,0.0}},
		0,
		{0.0},
		2.0
	},
	{
		"multiplication log subexpression",
		"log( 10 * x) * (3 + 6 + 3 -1)",
		{{"x",10.0},{0,0.0}},
		0,
		{0.0},
		22.0
	},
	{
		"BM25",
		"w * log( (N - _1 + 0.5) / (_1 + 0.5)) * (_0 * (k1 + 1.0)) / (_0 + k1 * (1.0 - b + b * ((_2+1) / avgdoclen)))",
		{{"w",1.3},{"N",100000},{"k1",1.5},{"b",0.75},{"avgdoclen",200},{0,0.0}},
		3,
		{3/*ff*/,12345/*df*/,123/*doclen*/},
		2.0380324748
	},
	{
		0,
		0,
		{{0,0.0}},
		0,
		{0.0},
		0.0
	}
};

int main( int, const char**)
{
	g_errorhnd = createErrorBuffer_standard( 0, 1);
	if (!g_errorhnd)
	{
		std::cerr << "failed to create error buffer for scalar function test" << std::endl;
		return -1;
	}
	try
	{
		std::auto_ptr<ScalarFunctionParserInterface> parser( createScalarFunctionParser( g_errorhnd));
		unsigned int ii = 0;
		for (; tests[ii].name; ++ii)
		{
			if (!run( parser.get(), ii+1, tests[ii]))
			{
				throw std::runtime_error( "test failed");
			}
			else
			{
				std::cerr << "OK [" << (ii+1) << "] test '" << tests[ii].name << "'" << std::endl;
			}
		}
		if (g_errorhnd->hasError())
		{
			throw strus::runtime_error( "uncaught error: %s", g_errorhnd->fetchError());
		}
		std::cerr << "successfully executed " << ii << " tests." << std::endl;
		delete g_errorhnd;
		return 0;
	}
	catch (const std::exception& err)
	{
		if (g_errorhnd->hasError())
		{
			std::cerr << "EXCEPTION " << g_errorhnd->fetchError() << ": " << err.what() << std::endl;
		}
		else
		{
			std::cerr << "EXCEPTION " << err.what() << std::endl;
		}
	}
	return -1;
}

