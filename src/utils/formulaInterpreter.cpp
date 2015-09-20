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
#include "private/formulaInterpreter.hpp"
#include "private/internationalization.hpp"
#include <cmath> 
#include <limits> 
#include <algorithm>
#include <string> 
#include <sstream> 
#include <iostream> 
#include <iomanip> 
#include <stdint.h> 

using namespace strus;

static std::string lowercase( const std::string& name)
{
	std::string rt;
	rt = name;
	std::transform( rt.begin(), rt.end(), rt.begin(), ::tolower);
	return rt;
}

void FormulaInterpreter::Context::defineVariableMap( const std::string& name, VariableMap func)
{
	m_varmap[ lowercase(name)] = func;
}

FormulaInterpreter::VariableMap FormulaInterpreter::Context::getVariableMap( const std::string& name) const
{
	std::map<std::string,VariableMap>::const_iterator vi = m_varmap.find( name);
	if (vi == m_varmap.end()) throw strus::runtime_error(_TXT( "variable %s not defined"), name.c_str());
	return vi->second;
}

void FormulaInterpreter::Context::defineUnaryFunction( const std::string& name, FormulaInterpreter::UnaryFunction func)
{
	m_unaryfuncmap[ lowercase(name)] = func;
}

FormulaInterpreter::UnaryFunction FormulaInterpreter::Context::getUnaryFunction( const std::string& name) const
{
	std::map<std::string,UnaryFunction>::const_iterator vi = m_unaryfuncmap.find( name);
	if (vi == m_unaryfuncmap.end()) throw strus::runtime_error(_TXT( "unary function %s not defined"), name.c_str());
	return vi->second;
}

void FormulaInterpreter::Context::defineBinaryFunction( const std::string& name, FormulaInterpreter::BinaryFunction func)
{
	m_binaryfuncmap[ lowercase(name)] = func;
}

FormulaInterpreter::BinaryFunction FormulaInterpreter::Context::getBinaryFunction( const std::string& name) const
{
	std::map<std::string,BinaryFunction>::const_iterator vi = m_binaryfuncmap.find( name);
	if (vi == m_binaryfuncmap.end()) throw strus::runtime_error(_TXT( "binary function %s not defined"), name.c_str());
	return vi->second;
}

FormulaInterpreter::DimMap FormulaInterpreter::Context::getDimMap() const
{
	return m_dimmap;
}

static bool isAlpha( char ch)
{
	return ((ch|32) >= 'a' && (ch|32) <= 'z');
}
static bool isDigit( char ch)
{
	return ((ch) >= '0' && (ch) <= '9');
}
static bool isAlnum( char ch)
{
	return isAlpha(ch)|isDigit(ch)|(ch == '_');
}
static bool isSpace( char ch)
{
	return ((unsigned char)(ch) <= 32);
}
static bool isOperator( char ch)
{
	return ch=='*'||ch=='/'||ch=='+'||ch=='-'||ch=='%'||ch=='^';
}
static unsigned int operatorPrecedence( const std::string& op)
{
	if (op.size() != 1) return 0;
	if (op[0] == '^') return 4;
	if (op[0] == '%') return 3;
	if (op[0] == '*' || op[0] == '/') return 2;
	if (op[0] == '+' || op[0] == '-') return 1;
	return 0;
}



static void skipSpaces( std::string::const_iterator& si, const std::string::const_iterator& se)
{
	for (;si < se && isSpace(*si); ++si){}
}

static std::string parseIdentifier( std::string::const_iterator& si, const std::string::const_iterator& se)
{
	std::string rt;
	for (;si < se && isAlnum(*si); ++si) rt.push_back( ::tolower(*si));
	skipSpaces(si,se);
	return rt;
}
static std::string parseOperator( std::string::const_iterator& si, const std::string::const_iterator& se)
{
	std::string rt;
	if (si < se && isOperator(*si)) rt.push_back( *si++);
	skipSpaces(si,se);
	return rt;
}
static double parseNumber( std::string::const_iterator& si, const std::string::const_iterator& se)
{
	double rt = 0.0;
	bool sig = false;
	if (si < se && *si == '-')
	{
		sig = true;
		++si;
	}
	if (!isDigit(*si))
	{
		throw strus::runtime_error(_TXT("number expected"));
	}
	for (;si < se && isDigit(*si); ++si)
	{
		rt *= 10;
		rt += *si - '0';
	}
	if (si < se && *si == '.')
	{
		double frac = 1.0;
		for (++si; si < se && isDigit(*si); ++si)
		{
			frac /= 10;
			rt += (*si - '0') * frac;
		}
	}
	skipSpaces(si,se);
	return sig?-rt:rt;
}

unsigned int FormulaInterpreter::allocVariable( const std::string& name)
{
	m_strings.push_back( '\0');
	unsigned int rt = m_strings.size();
	m_strings.append( name);
	return rt;
}

void FormulaInterpreter::parseFunctionCall( const Context& context, const std::string& funcname, std::string::const_iterator& si, const std::string::const_iterator& se)
{
	unsigned int nofargs = parseSubExpression( context, si, se, ')');
	if (nofargs == 0)
	{
		VariableMap vm = context.getVariableMap( funcname);
		m_program.push_back( OpStruct( OpPushVar, vm));
	}
	else if (nofargs == 1)
	{
		UnaryFunction func = context.getUnaryFunction( funcname);
		m_program.push_back( OpStruct( OpUnaryFunction, func));
	}
	else if (nofargs == 2)
	{
		BinaryFunction func = context.getBinaryFunction( funcname);
		m_program.push_back( OpStruct( OpBinaryFunction, func));
	}
	else
	{
		throw strus::runtime_error( _TXT( "too many arguments for function '%s'"), funcname.c_str());
	}
}

void FormulaInterpreter::parseVariableExpression( const Context& context, std::string::const_iterator& si, const std::string::const_iterator& se)
{
	std::string var = parseIdentifier( si, se);
	if (*si == '(')
	{
		++si;
		parseFunctionCall( context, var, si, se);
	}
	else
	{
		VariableMap vm = context.getVariableMap( var);
		m_program.push_back( OpStruct( OpPushVar, vm));
	}
}

void FormulaInterpreter::parseOperand( const Context& context, std::string::const_iterator& si, const std::string::const_iterator& se)
{
	if (si == se)
	{
		throw strus::runtime_error( _TXT( "unexpected end of expression, operand expected"));
	}
	else if (isDigit(*si))
	{
		m_program.push_back( OpStruct( OpPushConst, parseNumber( si, se)));
	}
	else if (*si == '-')
	{
		std::string::const_iterator fi = si;
		++fi; skipSpaces( fi, se);
		if (fi < se)
		{
			if (isAlpha(*fi))
			{
				// Parse variable reference or function call:
				si = fi;
				parseVariableExpression( context, si, se);
				UnaryFunction func = context.getUnaryFunction( "-");
				m_program.push_back( OpStruct( OpUnaryFunction, func));
			}
			else if (*fi == '(')
			{
				si = fi; ++si; skipSpaces( si, se);
				parseFunctionCall( context, "-", si, se);
			}
			else if (isDigit(*fi))
			{
				m_program.push_back( OpStruct( OpPushConst, parseNumber( si, se)));
			}
			else
			{
				throw strus::runtime_error( _TXT( "expression expected after unari '-' operator"));
			}
		}
	}
	else if (*si == '#')
	{
		// Parse set dimension request:
		++si; skipSpaces( si, se);
		if (!isAlpha( *si)) throw strus::runtime_error( _TXT( "identifier expected after dimension operator '#'"));
		std::string var = parseIdentifier( si, se);
		m_program.push_back( OpStruct( OpPushDim, allocVariable( var)));
	}
	else if (isAlpha( *si))
	{
		// Parse variable reference or function call:
		parseVariableExpression( context, si, se);
	}
	else if (*si == '<')
	{
		// Parse loop <aggfunc,type>{ ... }:
		++si;
		if (!isAlpha( *si)) throw strus::runtime_error( _TXT( "tuple <aggfunc,type> expected in loop predicate (after '<')"));
		std::string aggregatorid = parseIdentifier( si, se);
		if (si == se || *si != ',') throw strus::runtime_error( _TXT( "tuple <aggfunc,type> expected in loop predicate (after '<')"));
		++si;
		std::string type = parseIdentifier( si, se);
		double initval = 0.0;
		if (si < se && *si == ',')
		{
			++si; skipSpaces( si, se);
			if (*si == '-' || isDigit(*si))
			{
				initval = parseNumber( si, se);
			}
			else
			{
				throw strus::runtime_error( _TXT( "number expected as third argument of loop predicate <aggfunc,type,initval>"));
			}
		}
		if (si == se || *si != '>') throw strus::runtime_error( _TXT( "tuple <aggfunc,type> expected in loop predicate (after '<')"));
		++si; skipSpaces( si, se);
		if (si == se || *si != '{') throw strus::runtime_error( _TXT( "open loop bracket '{' expected after loop predicate <aggfunc,type>"));
		++si; skipSpaces( si, se);
		BinaryFunction aggregatorf = context.getBinaryFunction( aggregatorid);

		m_program.push_back( OpStruct( OpLoop, allocVariable( type)));
		m_program.push_back( OpStruct( OpPushConst, initval));
		m_program.push_back( OpStruct( OpMark));

		unsigned int loopsize = parseSubExpression( context, si, se, '}');
		if (loopsize == 0) throw strus::runtime_error( _TXT( "content of loop '{...}' is empty"));
		if (loopsize > 1) throw strus::runtime_error( _TXT( "content of loop '{...}' has more than one expression element"));

		m_program.push_back( OpStruct( OpBinaryFunction, aggregatorf));
		m_program.push_back( OpStruct( OpAgain));
	}
	else
	{
		throw strus::runtime_error( _TXT( "function or variable itentifier or numeric operand expected"));
	}
}

unsigned int FormulaInterpreter::parseSubExpression( const Context& context, std::string::const_iterator& si, const std::string::const_iterator& se, char eb)
{
	unsigned int rt = 0;
	skipSpaces(si,se);
	if (si == se || *si == eb)
	{
		return 0;
	}
	while (si < se)
	{
		parseOperand( context, si, se);
		if (si < se)
		{
			if (*si == ',')
			{
				// Parse next argument:
				++si; skipSpaces( si, se);
				++rt;
				if (si == se || *si == eb)
				{
					throw strus::runtime_error( _TXT( "unexpected end of expression"));
				}
			}
			else if (isOperator( *si))
			{
				std::string op = parseOperator( si, se);
				parseOperand( context, si, se);
				if (si < se && isOperator(*si))
				{
					std::string::const_iterator fi = si;
					std::string opnext = parseOperator( fi, se);
					if (operatorPrecedence(op) != operatorPrecedence(opnext))
					{
						throw strus::runtime_error( _TXT( "mixing operators with different precedence without grouping them with brackets '(' ')'"));
					}
				}
				BinaryFunction opfunc = context.getBinaryFunction( op);
				m_program.push_back( OpStruct( OpBinaryFunction, opfunc));
			}
		}
	}
	return rt;
}

FormulaInterpreter::FormulaInterpreter( const Context& context, const std::string& source)
	:m_dimmap(context.getDimMap())
{
	std::string::const_iterator si = source.begin(), se = source.end();
	try
	{
		unsigned int exprsize = parseSubExpression( context, si, se, '\0');
		if (exprsize > 1)
		{
			throw strus::runtime_error( _TXT("program with more than one return value"));
		}
		if (exprsize == 0)
		{
			throw strus::runtime_error( _TXT("program is empty"));
		}
	}
	catch (const std::bad_alloc&)
	{
		throw strus::runtime_error( _TXT("out of memory parsing formula"));
	}
	catch (const std::runtime_error& err)
	{
		std::size_t locidx = (std::size_t)(si - source.begin());
		std::string locstr( source);
		locstr.insert( locstr.begin()+locidx, '|');
		throw strus::runtime_error( _TXT("error in formula: %s (location %s)"), err.what(), locstr.c_str());
	}
}

template <typename Element, std::size_t Size>
class Stack
{
public:
	Stack() :m_itr(Size){}

	void push( Element elem)
	{
		if (m_itr == 0) throw strus::runtime_error( _TXT("stack overflow"));
		m_ar[ --m_itr] = elem;
	}

	Element pop()
	{
		if (m_itr == Size) throw strus::runtime_error( _TXT("logic error: pop from empty stack"));
		return m_ar[ m_itr++];
	}

	Element& back()
	{
		if (m_itr == Size) throw strus::runtime_error( _TXT("logic error: access top element from empty stack"));
		return m_ar[ m_itr];
	}

	bool empty() const
	{
		return m_itr == Size;
	}

private:
	Element m_ar[ Size];
	std::size_t m_itr;
};

struct LoopContext
{
	const char* type;
	unsigned int loopcnt;
	unsigned int loopsize;

	LoopContext( const LoopContext& o)
		:type(o.type),loopcnt(o.loopcnt),loopsize(o.loopsize){}
	LoopContext( const char* type_, unsigned int loopsize_)
		:type(type_),loopcnt(0),loopsize(loopsize_){}
	LoopContext()
		:type(0),loopcnt(0),loopsize(0){}

	bool next()
	{
		if (loopcnt+1 == loopsize) return false;
		++loopcnt;
		return true;
	}
};

double FormulaInterpreter::run( void* ctx) const
{
	Stack<double,256> stack;
	Stack<std::size_t,16> mark;
	Stack<LoopContext,16> loopctx;
	std::size_t ip = 0;

	while (ip < m_program.size())
	{
		const OpStruct& op = m_program[ ip];
		switch (op.opCode)
		{
			case OpMark:
			{
				mark.push( ip++);
				break;
			}
			case OpLoop:
			{
				int dim = (*m_dimmap)( ctx, m_strings.c_str() + op.operand.idx);
				if (dim <= 0)
				{
					int brkcnt = 1;
					for (;ip < m_program.size(); ++ip)
					{
						const OpStruct& xp = m_program[ ip];
						if (xp.opCode == OpLoop)
						{
							++brkcnt;
						}
						else if (xp.opCode == OpAgain)
						{
							--brkcnt;
							if (brkcnt == 0)
							{
								break;
							}
						}
					}
					if (ip == m_program.size())
					{
						throw strus::runtime_error(_TXT("illegal program code: end of loop not found"));
					}
				}
				else {
					loopctx.push( LoopContext( m_strings.c_str() + op.operand.idx, (unsigned int)dim));
				}
				++ip;
				break;
			}
			case OpAgain:
			{
				if (loopctx.back().next())
				{
					ip = mark.back();
				}
				else
				{
					loopctx.pop();
				}
				++ip;
				break;
			}
			case OpPushConst:
			{
				stack.push( op.operand.value);
				++ip;
				break;
			}
			case OpPushVar:
				if (loopctx.empty())
				{
					(*op.operand.variableMap)( ctx, 0, 0);
				}
				else
				{
					(*op.operand.variableMap)( ctx, loopctx.back().type, loopctx.back().loopcnt);
				}
				++ip;
				break;
			case OpPushDim:
			{
				int dim = (*m_dimmap)( ctx, m_strings.c_str() + op.operand.idx);
				if (dim < 0) dim = 0;
				stack.push( (double)dim);
				++ip;
				break;
			}
			case OpUnaryFunction:
			{
				stack.push( (*op.operand.unaryFunction)( stack.pop()));
				++ip;
				break;
			}
			case OpBinaryFunction:
			{
				double arg1 = stack.pop();
				double arg2 = stack.pop();
				stack.push( (*op.operand.binaryFunction)( arg1, arg2));
				++ip;
				break;
			}
		}
	}
	double rt = stack.pop();
	if (!stack.empty())
	{
		throw strus::runtime_error(_TXT("illegal program code: program stack not empty after completion of program run"));
	}
	return rt;
}

void FormulaInterpreter::OpStruct::print( std::ostream& out, const std::string& strings) const
{
	out << FormulaInterpreter::opCodeName( opCode);
	switch (opCode)
	{
		case OpMark:
			break;
		case OpLoop:
			out << " " << (strings.c_str() + operand.idx);
			break;
		case OpAgain:
			break;
		case OpPushConst:
		{
			std::ostringstream num; 
			num << std::setw(6) << std::setprecision(5) << operand.value;
			out << num.str();
			break;
		}
		case OpPushVar:
		{
			std::ostringstream msg;
			msg << std::hex << (uintptr_t)operand.variableMap;
			out << msg.str();
			break;
		}
		case OpPushDim:
			out << " " << (strings.c_str() + operand.idx);
			break;
		case OpUnaryFunction:
		{
			std::ostringstream msg;
			msg << std::hex << (uintptr_t)operand.unaryFunction;
			out << msg.str();
			break;
		}
		case OpBinaryFunction:
		{
			std::ostringstream msg;
			msg << std::hex << (uintptr_t)operand.binaryFunction;
			out << msg.str();
			break;
		}
	}
}

void FormulaInterpreter::print( std::ostream& out) const
{
	std::size_t ip = 0;

	while (ip < m_program.size())
	{
		const OpStruct& op = m_program[ ip];
		op.print( out, m_strings);
		out << std::endl;
	}
}


