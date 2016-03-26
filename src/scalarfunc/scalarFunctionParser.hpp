/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Default implementation of a scalar function parser
/// \file scalarFunctionParser.hpp
#ifndef _STRUS_SCALAR_FUNCTION_PARSER_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_SCALAR_FUNCTION_PARSER_IMPLEMENTATION_HPP_INCLUDED
#include "strus/scalarFunctionParserInterface.hpp"
#include <map>

namespace strus
{
/// \brief Forward declaration
class ScalarFunctionInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Default implementation of the scalar function parser
class ScalarFunctionParser
	:public ScalarFunctionParserInterface
{
public:
	explicit ScalarFunctionParser( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~ScalarFunctionParser(){}

	virtual void defineUnaryFunction( const std::string& name, UnaryFunction func);
	virtual void defineBinaryFunction( const std::string& name, BinaryFunction func);
	virtual void defineNaryFunction( const std::string& name, NaryFunction func);

	virtual ScalarFunctionInterface* createFunction( const std::string& src) const;

private:
	typedef std::map<std::string,BinaryFunction> BinaryFunctionMap;
	typedef std::map<std::string,UnaryFunction> UnaryFunctionMap;
	typedef std::map<std::string,NaryFunction> NaryFunctionMap;

	ErrorBufferInterface* m_errorhnd;
	BinaryFunctionMap m_binaryFunctionMap;
	UnaryFunctionMap m_unaryFunctionMap;
	NaryFunctionMap m_naryFunctionMap;
};

}// namespace
#endif

