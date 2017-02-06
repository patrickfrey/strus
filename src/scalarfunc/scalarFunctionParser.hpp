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
class ScalarFunction;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Default implementation of the scalar function parser
class ScalarFunctionParser
	:public ScalarFunctionParserInterface
{
public:
	/// \brief Constructor
	explicit ScalarFunctionParser( ErrorBufferInterface* errorhnd_);

	virtual ~ScalarFunctionParser(){}

	typedef double (*BinaryFunction)( double arg1, double arg2);
	typedef double (*UnaryFunction)( double arg);
	typedef double (*NaryFunction)( unsigned int nofargs, const double* args);

	/// \brief Define a binary function by name
	/// \param[in] name name of the function
	/// \param[in] func pointer to the function
	void defineUnaryFunction(
			const std::string& name, UnaryFunction func);

	/// \brief Define an unary function by name
	/// \param[in] name name of the function
	/// \param[in] func pointer to the function
	void defineBinaryFunction(
			const std::string& name, BinaryFunction func);

	/// \brief Define an N-ary function by name
	/// \param[in] name name of the function
	/// \param[in] func pointer to the function
	/// \param[in] min_nofargs minimum number of arguments
	/// \param[in] max_nofargs maximum number of arguments
	void defineNaryFunction(
			const std::string& name, NaryFunction func,
			unsigned int min_nofargs, unsigned int max_nofargs);

	virtual ScalarFunctionInterface* createFunction(
			const std::string& src,
			const std::vector<std::string>& argumentNames) const;

private:
	typedef std::map<std::string,std::size_t> ArgumentNameMap;
	struct ParserContext
	{
		ArgumentNameMap argumentNameMap;
		std::size_t nofNamedArguments;
	};

	void parseOperand( ScalarFunction* func, ParserContext* ctx, std::string::const_iterator& si, const std::string::const_iterator& se) const;
	void parseExpression( ScalarFunction* func, ParserContext* ctx, unsigned int oprPrecedence, std::string::const_iterator& si, const std::string::const_iterator& se) const;
	void parseFunctionCall( ScalarFunction* func, ParserContext* ctx, const std::string& functionName, std::string::const_iterator& si, const std::string::const_iterator& se) const;
	void resolveFunctionCall( ScalarFunction* func, const std::string& functionName, unsigned int nofArguments) const;
	void resolveIdentifier( ScalarFunction* func, ParserContext* ctx, const std::string& var) const;

private:
	struct NaryFunctionDef
	{
		unsigned int min_nofargs;
		unsigned int max_nofargs;
		NaryFunction func;

		NaryFunctionDef()
			:min_nofargs(0),max_nofargs(0),func(0){}
		NaryFunctionDef( unsigned int min_nofargs_, unsigned int max_nofargs_, NaryFunction func_)
			:min_nofargs(min_nofargs_),max_nofargs(max_nofargs_),func(func_){}
		NaryFunctionDef( const NaryFunctionDef& o)
			:min_nofargs(o.min_nofargs),max_nofargs(o.max_nofargs),func(o.func){}
	};

	typedef std::map<std::string,BinaryFunction> BinaryFunctionMap;
	typedef std::map<std::string,UnaryFunction> UnaryFunctionMap;
	typedef std::map<std::string,NaryFunctionDef> NaryFunctionMap;

	ErrorBufferInterface* m_errorhnd;
	BinaryFunctionMap m_binaryFunctionMap;
	UnaryFunctionMap m_unaryFunctionMap;
	NaryFunctionMap m_naryFunctionMap;
};

}// namespace
#endif

