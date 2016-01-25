/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013,2014 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "formulaInterpreter.hpp"
#include <stdexcept>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>

using namespace strus;

class Context
{
public:
	Context(){}

	void defineFeature( const char* name, double df, double tf)
	{
		std::size_t idx;
		std::map<std::string,std::size_t>::const_iterator ti = m_sets.find( name);
		if (ti == m_sets.end())
		{
			m_sets[ name] = idx = m_features.size();
			m_features.push_back( FeatureVector());
		}
		else
		{
			idx = ti->second;
		}
		m_features[ idx].push_back( Feature( df, tf));
	}

	void defineParameter( double val)
	{
		m_x_ar.push_back( val);
	}

private:
	struct Feature
	{
		Feature( const Feature& o)
			:df(o.df),tf(o.tf){}
		Feature( double df_, double tf_)
			:df(df_),tf(tf_){}

		double df;
		double tf;
	};
	friend class FunctionMap;
	typedef std::vector<Feature> FeatureVector;
	std::vector<FeatureVector> m_features;
	std::map<std::string,std::size_t> m_sets;
	std::vector<double> m_x_ar;
};


class FunctionMap
	:public FormulaInterpreter::FunctionMap
{
public:
	FunctionMap()
		:FormulaInterpreter::FunctionMap( &dimMap)
	{
		std::size_t xi=0,xe=10;
		for (; xi < xe; ++xi)
		{
			char nam[3];
			nam[0] = 'x';
			nam[1] = (char)(xi+'0');
			nam[2] = '\0';
			defineVariableMap( nam, FormulaInterpreter::VariableMap( &variableMap_xi, xi));
		}
		defineVariableMap( "df", &variableMap_df);
		defineVariableMap( "tf", &variableMap_tf);
		defineUnaryFunction( "log", &unaryFunction_log10);
		defineUnaryFunction( "-", &unaryFunction_minus);
		defineBinaryFunction( "-", &binaryFunction_minus);
		defineBinaryFunction( "+", &binaryFunction_plus);
		defineBinaryFunction( "*", &binaryFunction_mul);
		defineBinaryFunction( "/", &binaryFunction_div);
		defineWeightingFunction( "minwin", weightingFunction_minwin);
	}

private:
	static FormulaInterpreter::IteratorSpec dimMap( void* ctx, const char* type)
	{
		Context* THIS = (Context*)ctx;
		std::map<std::string,std::size_t>::const_iterator ti = THIS->m_sets.find( type);
		if (ti == THIS->m_sets.end()) return FormulaInterpreter::IteratorSpec();
		return FormulaInterpreter::IteratorSpec( ti->second, THIS->m_features[ ti->second].size());
	}
	static double variableMap_xi( void* ctx, int typeidx, unsigned int idx)
	{
		Context* THIS = (Context*)ctx;
		if (idx >= THIS->m_x_ar.size()) return std::numeric_limits<double>::quiet_NaN();
		return THIS->m_x_ar[idx];
	}
	static double variableMap_df( void* ctx, int typeidx, unsigned int idx)
	{
		Context* THIS = (Context*)ctx;
		if (typeidx < 0) return std::numeric_limits<double>::quiet_NaN();
		if (idx >= THIS->m_features[ typeidx].size()) return std::numeric_limits<double>::quiet_NaN();
		return THIS->m_features[ typeidx][idx].df;
	}
	static double variableMap_tf( void* ctx, int typeidx, unsigned int idx)
	{
		Context* THIS = (Context*)ctx;
		if (typeidx < 0) return std::numeric_limits<double>::quiet_NaN();
		if (idx >= THIS->m_features[ typeidx].size()) return std::numeric_limits<double>::quiet_NaN();
		return THIS->m_features[ typeidx][idx].tf;
	}
	static double unaryFunction_minus( double arg)
	{
		return -arg;
	}
	static double unaryFunction_log10( double arg)
	{
		return std::log( arg);
	}
	static double binaryFunction_minus( double arg1, double arg2)
	{
		return arg1 - arg2;
	}
	static double binaryFunction_plus( double arg1, double arg2)
	{
		return arg1 + arg2;
	}
	static double binaryFunction_mul( double arg1, double arg2)
	{
		return arg1 * arg2;
	}
	static double binaryFunction_div( double arg1, double arg2)
	{
		return arg1 / arg2;
	}
	static double weightingFunction_minwin( void* ctx, int typeidx, int range, int cardinality)
	{
		Context* THIS = (Context*)ctx;
		if (typeidx < 0) return std::numeric_limits<double>::quiet_NaN();
		int size = THIS->m_features[ typeidx].size();
		return (double)(size + range * 2) / (double)cardinality;
	}
};

struct FeatureDef
{
	const char* name;
	double df;
	double tf;
};

struct Test
{
	const char* name;
	const char* formula;
	FeatureDef feats[20];
	double parameter[4];
	double result;
};

static bool run( unsigned int testidx, const Test& test)
{
	static const FunctionMap functionMap;
	Context context;
	FeatureDef const* fi = test.feats;
	for (;fi->name; ++fi)
	{
		context.defineFeature( fi->name, fi->df, fi->tf);
	}
	std::size_t xi = 0, xe = 4;
	for (; xi<xe; ++xi)
	{
		context.defineParameter( test.parameter[xi]);
	}
	FormulaInterpreter interp( functionMap, test.formula);
	double result = interp.run( &context);
	double xx = (result - test.result);
	bool rt = (xx * xx < std::numeric_limits<double>::epsilon());
	if (!rt)
	{
		std::cerr << "[" << testidx << "] test '" << test.name << "'" << std::endl;
		interp.print( std::cerr);

		std::cerr << "result = " << result << " expected = " << test.result << std::endl;
	}
	return rt;
}

static Test tests[] =
{
	{
		"parameter variable",
		"x1",
		{{"stemmed",1.0,1.0},{"unstemmed",1.0,1.0},{"unstemmed",1.0,1.0},{0,0.0,0.0}},
		{0.1, 0.2, 0.3, 0.4},
		0.2
	},
	{
		"parameter variable sum",
		"x1 + x3",
		{{"stemmed",1.0,1.0},{"unstemmed",1.0,1.0},{"unstemmed",1.0,1.0},{0,0.0,0.0}},
		{0.1, 0.2, 0.3, 0.4},
		0.6
	},
	{
		"dimension variable",
		"#unstemmed",
		{{"stemmed",1.0,1.0},{"unstemmed",1.0,1.0},{"unstemmed",1.0,1.0},{0,0.0,0.0}},
		{0.0},
		2
	},
	{
		"minus dimension variable",
		"-#unstemmed",
		{{"unstemmed",1.0,1.0},{"unstemmed",1.0,1.0},{0,0.0,0.0}},
		{0.0},
		-2
	},
	{
		"difference dimension variables",
		"#stemmed -#unstemmed",
		{{"stemmed",1.0,1.0},{"stemmed",1.0,1.0},{"unstemmed",1.0,1.0},{0,0.0,0.0}},
		{0.0},
		1
	},
	{
		"sum of 1 variable",
		"<+,unstemmed,0>{df}",
		{{"stemmed",1.1,2.1},{"stemmed",1.2,2.2},{"unstemmed",1.3,2.3},{0,0.0,0.0}},
		{0.0},
		1.3
	},
	{
		"sum of 2 variables",
		"<+,stemmed,0.4>{df}",
		{{"stemmed",1.1,2.1},{"stemmed",1.2,2.2},{"unstemmed",1.3,2.3},{0,0.0,0.0}},
		{0.0},
		2.7
	},
	{
		"sum of expressions",
		"<+,stemmed,0.4>{df * tf}",
		{{"stemmed",1.1,1.2},{"stemmed",2.1,2.2},{"unstemmed",3.1,3.2},{0,0.0,0.0}},
		{0.0},
		6.34
	},
	{
		"sum of expressions with negative operand",
		"< +, stemmed, 0.5>{ df*-tf} ",
		{{"stemmed",1.1,1.2},{"stemmed",2.1,2.2},{"unstemmed",3.1,3.2},{0,0.0,0.0}},
		{0.0},
		-5.44
	},
	{
		"addition of 2 sums of expressions [1]",
		"< +, stemmed, 0.5>{ df*tf} + (<+,unstemmed>{ tf} / #unstemmed)",
		{{"stemmed",1.1,1.2},{"stemmed",2.1,2.2},{"unstemmed",3.1,3.2},{0,0.0,0.0}},
		{0.0},
		9.64
	},
	{
		"addition of 2 sums of expressions [2]",
		"(<+,stemmed,0.5>{ df*tf} / #stemmed) + (<+,unstemmed>{ tf} / #unstemmed)",
		{{"stemmed",1.1,1.2},{"stemmed",2.1,2.2},{"unstemmed",3.1,3.2},{0,0.0,0.0}},
		{0.0},
		6.42
	},
	{
		"min win parameter passing [1]",
		"minwin( stemmed, 17, 32)",
		{{"stemmed",1.1,1.2},{"stemmed",2.1,2.2},{"unstemmed",3.1,3.2},{0,0.0,0.0}},
		{0.0},
		1.125
	},
	{0,0,{{0,0.0,0.0}},{0.0},0.0}
};

int main( int, const char**)
{
	try
	{
		unsigned int ii = 0;
		for (; tests[ii].name; ++ii)
		{
			if (!run( ii+1, tests[ii]))
			{
				throw std::runtime_error( "test failed");
			}
			else
			{
				std::cerr << "OK [" << (ii+1) << "] test '" << tests[ii].name << "'" << std::endl;
			}
		}
		std::cerr << "successfully executed " << ii << " tests." << std::endl;
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


