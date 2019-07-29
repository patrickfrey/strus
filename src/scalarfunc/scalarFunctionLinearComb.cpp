/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Exported functions of the strus scalarfunc library
/// \file "scalarFunctionLinearComb.cpp"
#include "scalarFunctionLinearComb.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/structView.hpp"
#include "scalarFunction.hpp"
#include "scalarFunctionInstance.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace strus;

class ScalarFunctionInstance_linearcomb
	:public ScalarFunctionInstanceInterface
{
public:
	ScalarFunctionInstance_linearcomb( const std::vector<double>& factors_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_factors(factors_){}

	virtual ~ScalarFunctionInstance_linearcomb(){}

	virtual void setVariableValue( const std::string& name_, double value)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables defined in plain linear combination"));
	}

	virtual double call( const double* args, unsigned int nofargs) const
	{
		double rt = 0.0;
		std::vector<double>::const_iterator fi = m_factors.begin(), fe = m_factors.end();
		unsigned int ai = 0;
		for (; fi != fe && ai < nofargs; ++ai,++fi)
		{
			rt += *fi * args[ai];
		}
		for (; ai < nofargs; ++ai)
		{
			rt += args[ai];
		}
		return rt;
	}

	virtual const char* name() const {return "linearcomb";}

	virtual StructView view() const
	{
		try
		{
			std::vector<int> ar;
			return StructView()("name","linearcomb")("bla",ar)("factors",m_factors);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error mapping scalar function linear combination to string: %s"), *m_errorhnd, std::string());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	std::vector<double> m_factors;
};

class ScalarFunction_linearcomb
	:public ScalarFunctionInterface
{
public:
	ScalarFunction_linearcomb( const std::vector<double>& factors_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_factors(factors_)
	{}
	virtual ~ScalarFunction_linearcomb(){}

	virtual std::vector<std::string> getVariables() const
	{
		return std::vector<std::string>();
	}

	virtual unsigned int getNofArguments() const
	{
		return m_factors.size();
	}

	virtual void setDefaultVariableValue( const std::string&, double)
	{
		m_errorhnd->report( ErrorCodeNotImplemented, _TXT("no variables defined in plain linear combination"));
	}

	virtual ScalarFunctionInstanceInterface* createInstance() const
	{
		try
		{
			return new ScalarFunctionInstance_linearcomb( m_factors, m_errorhnd);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error creating instance of linear combination scalar function: %s"), *m_errorhnd, 0);
	}

	virtual const char* name() const {return "linearcomb";}

	virtual StructView view() const
	{
		try
		{
			return StructView()("name","linearcomb")("factors",m_factors);
		}
		CATCH_ERROR_MAP_RETURN( _TXT("error mapping scalar function linear combination to string: %s"), *m_errorhnd, std::string());
	}
	
private:
	ErrorBufferInterface* m_errorhnd;
	std::vector<double> m_factors;
};


ScalarFunctionInterface* strus::createScalarFunction_linearcomb( const std::vector<double>& factors, ErrorBufferInterface* errorhnd)
{
	try
	{
		return new ScalarFunction_linearcomb( factors, errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating linear combination scalar function instance: %s"), *errorhnd, 0);
}
