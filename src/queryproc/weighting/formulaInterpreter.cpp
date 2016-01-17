/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "formulaInterpreter.hpp"
#include "private/internationalization.hpp"
#include <cmath> 
#include <limits> 
#include <algorithm>
#include <string> 
#include <sstream> 
#include <iostream> 
#include <iomanip> 
#include <stdint.h> 

#undef STRUS_LOWLEVEL_DEBUG

using namespace strus;

static std::string lowercase( const std::string& name)
{
	std::string rt;
	rt = name;
	std::transform( rt.begin(), rt.end(), rt.begin(), ::tolower);
	return rt;
}

void FormulaInterpreter::FunctionMap::defineVariableMap( const std::string& name, VariableMap func)
{
	m_varmap[ lowercase(name)] = func;
}

FormulaInterpreter::VariableMap FormulaInterpreter::FunctionMap::getVariableMap( const std::string& name) const
{
	std::map<std::string,VariableMap>::const_iterator vi = m_varmap.find( name);
	if (vi == m_varmap.end()) throw strus::runtime_error(_TXT( "variable '%s' not defined"), name.c_str());
	return vi->second;
}

void FormulaInterpreter::FunctionMap::defineUnaryFunction( const std::string& name, FormulaInterpreter::UnaryFunction func)
{
	m_unaryfuncmap[ lowercase(name)] = func;
	m_namemap[ (uintptr_t)func] = name;
}

FormulaInterpreter::UnaryFunction FormulaInterpreter::FunctionMap::getUnaryFunction( const std::string& name) const
{
	std::map<std::string,UnaryFunction>::const_iterator vi = m_unaryfuncmap.find( name);
	if (vi == m_unaryfuncmap.end()) throw strus::runtime_error(_TXT( "unary function '%s' not defined"), name.c_str());
	return vi->second;
}

const std::string& FormulaInterpreter::FunctionMap::getUnaryFunctionName( UnaryFunction func) const
{
	std::map<uintptr_t,std::string>::const_iterator ni = m_namemap.find( (uintptr_t)func);
	if (ni == m_namemap.end()) throw strus::runtime_error(_TXT("name of unary function not defined"));
	return ni->second;
}

void FormulaInterpreter::FunctionMap::defineBinaryFunction( const std::string& name, FormulaInterpreter::BinaryFunction func)
{
	m_binaryfuncmap[ lowercase(name)] = func;
	m_namemap[ (uintptr_t)func] = name;
}

FormulaInterpreter::BinaryFunction FormulaInterpreter::FunctionMap::getBinaryFunction( const std::string& name) const
{
	std::map<std::string,BinaryFunction>::const_iterator vi = m_binaryfuncmap.find( name);
	if (vi == m_binaryfuncmap.end()) throw strus::runtime_error(_TXT( "binary function '%s' not defined"), name.c_str());
	return vi->second;
}

const std::string& FormulaInterpreter::FunctionMap::getBinaryFunctionName( BinaryFunction func) const
{
	std::map<uintptr_t,std::string>::const_iterator ni = m_namemap.find( (uintptr_t)func);
	if (ni == m_namemap.end()) throw strus::runtime_error(_TXT("name of binary function not defined"));
	return ni->second;
}

FormulaInterpreter::IteratorMap FormulaInterpreter::FunctionMap::getIteratorMap() const
{
	return m_iteratorMap;
}

std::string FormulaInterpreter::FunctionMap::tostring() const
{
	std::string rt;
	rt.append( "variables: ");
	std::map<std::string,VariableMap>::const_iterator vi = m_varmap.begin(), ve = m_varmap.end();
	for (int vidx=0; vi != ve; ++vi,++vidx)
	{
		if (vidx) rt.append( " ");
		rt.append( vi->first);
	}
	rt.append( "\nunary functions: ");
	std::map<std::string,UnaryFunction>::const_iterator ui = m_unaryfuncmap.begin(), ue = m_unaryfuncmap.end();
	for (int uidx=0; ui != ue; ++ui,++uidx)
	{
		if (uidx) rt.append( " ");
		rt.append( ui->first);
	}
	rt.append( "\nbinary functions: ");
	std::map<std::string,BinaryFunction>::const_iterator bi = m_binaryfuncmap.begin(), be = m_binaryfuncmap.end();
	for (int bidx=0; bi != be; ++bi,++bidx)
	{
		if (bidx) rt.append( " ");
		rt.append( bi->first);
	}
	rt.append("\n");
	return rt;
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

void FormulaInterpreter::parseFunctionCall( const FunctionMap& functionMap, const std::string& funcname, std::string::const_iterator& si, const std::string::const_iterator& se)
{
	unsigned int nofargs = parseSubExpression( functionMap, si, se, ')');
	if (nofargs == 0)
	{
		VariableMap vm = functionMap.getVariableMap( funcname);
		m_program.push_back( OpStruct( OpPushVar, (int)m_variablear.size()));
		m_variablear.push_back( vm);
	}
	else if (nofargs == 1)
	{
		UnaryFunction func = functionMap.getUnaryFunction( funcname);
		m_program.push_back( OpStruct( OpUnaryFunction, func));
	}
	else if (nofargs == 2)
	{
		BinaryFunction func = functionMap.getBinaryFunction( funcname);
		m_program.push_back( OpStruct( OpBinaryFunction, func));
	}
	else
	{
		throw strus::runtime_error( _TXT( "too many arguments for function '%s'"), funcname.c_str());
	}
}

void FormulaInterpreter::parseVariableExpression( const FunctionMap& functionMap, std::string::const_iterator& si, const std::string::const_iterator& se)
{
	std::string var = parseIdentifier( si, se);
	if (*si == '(')
	{
		++si;
		parseFunctionCall( functionMap, var, si, se);
	}
	else
	{
		VariableMap vm = functionMap.getVariableMap( var);
		m_program.push_back( OpStruct( OpPushVar, (int)m_variablear.size()));
		m_variablear.push_back( vm);
	}
}

void FormulaInterpreter::parseOperand( const FunctionMap& functionMap, std::string::const_iterator& si, const std::string::const_iterator& se)
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
			if (isAlpha(*fi) || *fi == '#' || *fi == '<')
			{
				// Parse variable reference or function call:
				si = fi;
				parseOperand( functionMap, si, se);
				UnaryFunction func = functionMap.getUnaryFunction( "-");
				m_program.push_back( OpStruct( OpUnaryFunction, func));
			}
			else if (*fi == '(')
			{
				si = fi; ++si; skipSpaces( si, se);
				parseFunctionCall( functionMap, "-", si, se);
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
	else if (*si == '(')
	{
		++si; skipSpaces(si,se);
		unsigned int exprsize = parseSubExpression( functionMap, si, se, ')');
		if (exprsize == 0) throw strus::runtime_error( _TXT( "content of operand '(...)' is empty"));
		if (exprsize > 1) throw strus::runtime_error( _TXT( "content of operand '(...)' has more than one element"));
	}
	else if (*si == '#')
	{
		// Parse set dimension request:
		++si; skipSpaces( si, se);
		if (!isAlpha( *si)) throw strus::runtime_error( _TXT( "identifier expected after dimension operator '#'"));
		std::string var = parseIdentifier( si, se);
		m_program.push_back( OpStruct( OpPushDim, (int)allocVariable( var)));
	}
	else if (isAlpha( *si))
	{
		// Parse variable reference or function call:
		parseVariableExpression( functionMap, si, se);
	}
	else if (*si == '<')
	{
		// Parse loop <aggfunc,type>{ ... }:
		std::string aggregatorid;
		++si; skipSpaces(si,se);
		if (isOperator( *si))
		{
			aggregatorid = parseOperator( si, se);
		}
		else if (isAlpha( *si))
		{
			aggregatorid = parseIdentifier( si, se);
		}
		else
		{
			throw strus::runtime_error( _TXT( "tuple <aggfunc,type> expected in loop predicate (after '<')"));
		}
		if (si == se || *si != ',') throw strus::runtime_error( _TXT( "tuple <aggfunc,type> expected in loop predicate (after '<')"));
		++si; skipSpaces(si,se);
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
		BinaryFunction aggregatorf = functionMap.getBinaryFunction( aggregatorid);

		m_program.push_back( OpStruct( OpLoop, (int)allocVariable( type)));
		m_program.push_back( OpStruct( OpPushConst, initval));
		m_program.push_back( OpStruct( OpMark));

		unsigned int loopsize = parseSubExpression( functionMap, si, se, '}');
		if (loopsize == 0) throw strus::runtime_error( _TXT( "content of loop '{...}' is empty"));
		if (loopsize > 1) throw strus::runtime_error( _TXT( "content of loop '{...}' has more than one expression element"));

		m_program.push_back( OpStruct( OpBinaryFunction, aggregatorf));
		m_program.push_back( OpStruct( OpAgain));
	}
	else
	{
		throw strus::runtime_error( _TXT( "function or variable identifier or numeric operand expected"));
	}
}

unsigned int FormulaInterpreter::parseSubExpression( const FunctionMap& functionMap, std::string::const_iterator& si, const std::string::const_iterator& se, char eb)
{
	unsigned int rt = 0;
	skipSpaces(si,se);
	if (si == se || *si == eb)
	{
		return 0;
	}
	while (si < se && *si != eb)
	{
		parseOperand( functionMap, si, se);
		++rt;
		while (si < se && isOperator( *si))
		{
			std::string op = parseOperator( si, se);
			parseOperand( functionMap, si, se);
			if (si < se && isOperator(*si))
			{
				std::string::const_iterator fi = si;
				std::string opnext = parseOperator( fi, se);
				if (operatorPrecedence(op) != operatorPrecedence(opnext))
				{
					throw strus::runtime_error( _TXT( "mixing operators with different precedence without grouping them with brackets '(' ')'"));
				}
			}
			BinaryFunction opfunc = functionMap.getBinaryFunction( op);
			m_program.push_back( OpStruct( OpBinaryFunction, opfunc));
		}
		if (si < se && *si == ',')
		{
			// Parse next argument:
			++si; skipSpaces( si, se);
			if (si == se || *si == eb)
			{
				throw strus::runtime_error( _TXT( "unexpected end of expression"));
			}
		}
	}
	if (si < se && *si == eb)
	{
		++si;
		skipSpaces(si,se);
	}
	return rt;
}

FormulaInterpreter::FormulaInterpreter( const FunctionMap& functionMap, const std::string& source)
	:m_funcmap(functionMap),m_iteratorMap(functionMap.getIteratorMap())
{
	std::string::const_iterator si = source.begin(), se = source.end();
	try
	{
		unsigned int exprsize = parseSubExpression( functionMap, si, se, '\0');
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
		std::string locstr;
		std::size_t restsize = source.size() - locidx;
		bool cutend = false;
		bool cutstart = false;
		if (restsize > 60)
		{
			restsize = 60;
			cutend = true;
		}
		if (locidx > 30)
		{
			locstr.append( source.c_str() + locidx - 30, restsize + 30);
			locidx = 30;
			cutstart = true;
		}
		else
		{
			locstr.append( source.c_str(), restsize);
		}
		locstr.insert( locidx, "<-- ! -->");
		if (cutstart)
		{
			locstr = std::string("...") + locstr;
		}
		if (cutend)
		{
			locstr.append("...");
		}
		throw strus::runtime_error( _TXT("error in formula: %s location: %s"), err.what(), locstr.c_str());
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

	void print( std::ostream& out) const
	{
		std::ostringstream msg;
		msg << std::setprecision(5);
		for (std::size_t ii = m_itr; ii<Size; ++ii)
		{
			if (ii > m_itr) msg << "|";
			msg << m_ar[ii];
		}
		out << msg.str();
	}

private:
	Element m_ar[ Size];
	std::size_t m_itr;
};

struct LoopContext
{
	unsigned int itr;
	FormulaInterpreter::IteratorSpec iteratorSpec;

	LoopContext( const LoopContext& o)
		:itr(o.itr),iteratorSpec(o.iteratorSpec){}
	explicit LoopContext( const FormulaInterpreter::IteratorSpec& iteratorSpec_)
		:itr(0),iteratorSpec(iteratorSpec_){}
	LoopContext()
		:itr(0){}

	bool next()
	{
		if (itr+1 >= iteratorSpec.size) return false;
		++itr;
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
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "[" << ip << "] ";
		op.print( std::cerr, m_strings, m_variablear, m_funcmap);
		std::cerr << "  stack [";
		stack.print( std::cerr);
		std::cerr << "]" << std::endl;
#endif
		switch (op.opCode)
		{
			case OpMark:
			{
				mark.push( ip++);
				break;
			}
			case OpLoop:
			{
				FormulaInterpreter::IteratorSpec iteratorSpec = (*m_iteratorMap)( ctx, m_strings.c_str() + op.operand.idx);
				if (!iteratorSpec.defined())
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
					loopctx.push( LoopContext( iteratorSpec));
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
			{
				const VariableMap& vm = m_variablear[ op.operand.idx];
				if (vm.idx >= 0)
				{
					stack.push( (*vm.function)( ctx, -1, vm.idx));
				}
				else if (loopctx.empty())
				{
					stack.push( (*vm.function)( ctx, -1, 0));
				}
				else
				{
					stack.push( (*vm.function)( ctx, loopctx.back().iteratorSpec.typeidx, loopctx.back().itr));
				}
				++ip;
				break;
			}
			case OpPushDim:
			{
				IteratorSpec iteratorSpec = (*m_iteratorMap)( ctx, m_strings.c_str() + op.operand.idx);
				if (iteratorSpec.defined())
				{
					stack.push( (double)iteratorSpec.size);
				}
				else
				{
					stack.push( 0.0);
				}
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
				double arg2 = stack.pop();
				double arg1 = stack.pop();
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

void FormulaInterpreter::OpStruct::print( std::ostream& out, const std::string& strings, const std::vector<VariableMap>& variablear, const FunctionMap& funcmap) const
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
			out << " " << num.str();
			break;
		}
		case OpPushVar:
		{
			std::ostringstream msg;
			const VariableMap& vm = variablear[ operand.idx];
			msg << " " << std::hex << (uintptr_t)vm.function;
			if (vm.idx >= 0) msg << " : " << std::dec << vm.idx;
			out << msg.str();
			break;
		}
		case OpPushDim:
			out << " " << (strings.c_str() + operand.idx);
			break;
		case OpUnaryFunction:
		{
			out << " " << funcmap.getUnaryFunctionName( operand.unaryFunction);
			break;
		}
		case OpBinaryFunction:
		{
			out << " " << funcmap.getBinaryFunctionName( operand.binaryFunction);
			break;
		}
	}
}

void FormulaInterpreter::print( std::ostream& out) const
{
	std::size_t ip = 0;

	for (;ip < m_program.size(); ++ip)
	{
		const OpStruct& op = m_program[ ip];
		op.print( out, m_strings, m_variablear, m_funcmap);
		out << std::endl;
	}
}


