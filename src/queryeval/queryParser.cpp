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
#include "queryParser.hpp"
#include "tupleGenerator.hpp"
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <boost/scoped_array.hpp>

using namespace strus;

typedef QueryParser::JoinOperation::Selector Selector;
typedef QueryParser::JoinOperation::SelectorSet SelectorSet;
typedef QueryParser::JoinOperation::SelectorSetR SelectorSetR;

static SelectorSetR calcSelectorSet( TupleGenerator::Mode genmode, const std::vector<SelectorSetR>& selset)
{
	SelectorSetR rt;
	std::size_t rowsize = 0;
	std::vector<SelectorSetR>::const_iterator si = selset.begin(), se = selset.end();
	for (; si != se; ++si)
	{
		if (!si->get() || (*si)->ar().size() == 0) return rt;
		rowsize += (*si)->rowsize();
	}
	if (rowsize == 0) return rt;

	rt.reset( new SelectorSet( rowsize));
	boost::scoped_array<Selector> curRef( new Selector[ rowsize]);
	Selector* cur = curRef.get();

	TupleGenerator gen( genmode);
	for (si = selset.begin(); si != se; ++si)
	{
		gen.defineColumn( (*si)->nofrows());
	}

	if (!gen.empty()) do
	{
		// Build product row:
		std::size_t cntidx = 0;
		std::size_t rowidx = 0;
		for (si = selset.begin(); si != se; ++si,++cntidx)
		{
			std::size_t ri = (*si)->rowsize() * gen.column( cntidx);
			for (std::size_t ci=0; ci<(*si)->rowsize(); ++ci)
			{
				cur[ rowidx + ci] = (*si)->ar()[ ri + ci];
			}
			rowidx += (*si)->rowsize();
		}
		if (rowidx != rowsize) throw std::logic_error("query parser assertion failed: rowidx != rowsize");
		rt->pushRow( cur);
	}
	while (gen.next());
	return rt;
}

static std::string errorPosition( const char* base, const char* itr)
{
	unsigned int line = 1;
	unsigned int col = 1;
	std::ostringstream msg;

	for (unsigned int ii=0,nn=itr-base; ii < nn; ++ii)
	{
		if (base[ii] == '\n')
		{
			col = 1;
			++line;
		}
		else
		{
			++col;
		}
	}
	msg << "at line " << line << " column " << col;
	return msg.str();
}



static bool isAlpha( char ch)
{
	return ((ch|32) <= 'z' && (ch|32) >= 'a') || (ch) == '_';
}
static bool isDigit( char ch)
{
	return (ch <= '9' && ch >= '0');
}
static bool isAlnum( char ch)
{
	return isAlpha(ch) || isDigit(ch);
}
static bool isAssign( char ch)
{
	return ch == '=';
}
static bool isColon( char ch)
{
	return ch == ':';
}
static bool isSemiColon( char ch)
{
	return ch == ';';
}
static bool isDot( char ch)
{
	return ch == '.';
}
static bool isComma( char ch)
{
	return ch == ',';
}
static bool isOpenSquareBracket( char ch)
{
	return ch == '[';
}
static bool isCloseSquareBracket( char ch)
{
	return ch == '[';
}
static bool isOpenOvalBracket( char ch)
{
	return ch == '(';
}
static bool isCloseOvalBracket( char ch)
{
	return ch == ')';
}
static bool isOpenAngleBracket( char ch)
{
	return ch == '<';
}
static bool isCloseAngleBracket( char ch)
{
	return ch == '>';
}
static bool isAsterisk( char ch)
{
	return ch == '*';
}
static bool isStringQuote( char ch)
{
	return ch == '\'' || ch == '"';
}
static bool isSpace( char ch)
{
	return ch == ' ' || ch == '\t' || ch == '\n';
}
static void skipSpaces( char const*& src)
{
	while (isSpace( *src)) src++;
}
static bool isEqual( const std::string& id, const char* idstr)
{
	char const* si = id.c_str();
	char const* di = idstr;
	for (; *si && *di && ((*si|32) == (*di|32)); ++si,++di){}
	return !*si && !*di;
}
static std::string IDENTIFIER( char const*& src)
{
	std::string rt;
	while (isAlnum( *src)) rt.push_back( *src++);
	skipSpaces( src);
	return rt;
}
static std::string STRING( char const*& src)
{
	std::string rt;
	char eb = *src++;
	while (*src != eb)
	{
		if (*src == '\0' || *src == '\n') throw std::runtime_error("unterminated string");
		if (*src == '\\')
		{
			src++;
			if (*src == '\0' || *src == '\n') throw std::runtime_error("unterminated string");
		}
		rt.push_back( *src++);
	}
	skipSpaces( src);
	return rt;
}
static unsigned int UNSIGNED( char const*& src)
{
	unsigned int rt = 0;
	while (isDigit( *src))
	{
		unsigned int vv = (rt * 10) + (*src - '0');
		if (vv <= rt) throw std::runtime_error( "index out of range");
		rt = vv;
		++src;
	}
	skipSpaces( src);
	return rt;
}
static double DOUBLE( char const*& src)
{
	unsigned int digitsAllowed = 20;
	double rt = 1.0;
	double div = 1.0;
	if (*src == '-')
	{
		++src;
		rt = -1.0;
	}
	while (isDigit( *src) && digitsAllowed)
	{
		rt = (rt * 10.0) + (*src - '0');
		++src;
		--digitsAllowed;
	}
	if (isDot( *src))
	{
		++src;
		while (isDigit( *src) && digitsAllowed)
		{
			div /= 10.0;
			rt = (rt * 10.0) + (*src - '0');
			++src;
			--digitsAllowed;
		}
	}
	if (!digitsAllowed)
	{
		throw std::runtime_error( "weight out of range");
	}
	skipSpaces( src);
	return rt * div;
}
static char OPERATOR( char const*& src)
{
	char rt = *src++;
	skipSpaces( src);
	return rt;
}
static int INTEGER( char const*& src)
{
	int rt = 0;
	int prev = 0;
	if (!*src) throw std::runtime_error("integer expected");
	bool neg = false;
	if (*src == '-')
	{
		++src;
		neg = true;
	}
	if (!(*src >= '0' && *src <= '9')) throw std::runtime_error("integer expected");

	for (; *src >= '0' && *src <= '9'; ++src)
	{
		rt = (rt * 10) + (*src - '0');
		if (prev > rt) throw std::runtime_error("integer number out of range");
		prev = rt;
	}
	if (isAlpha(*src)) throw std::runtime_error("integer expected");

	skipSpaces( src);
	if (neg)
	{
		return -rt;
	}
	else
	{
		return rt;
	}
}
static SelectorSetR SELECTORSET( char const*& src, strus::KeyMap<QueryParser::SetAttributes>& setmap)
{
	skipSpaces( src);
	enum SelectorFunction
	{
		ProductFunc,
		AscendingFunc,
		PermutationFunc,
		JoinFunc
	};
	SelectorFunction function = ProductFunc;
	unsigned int dim = 0;
	std::vector<SelectorSetR> selset;

	for (;;)
	{
		if (isAlpha( *src))
		{
			bool isFunction = false;
			char const* src_bk = src;
			std::string functionName( IDENTIFIER( src));
	
			if (isEqual( functionName, "product"))
			{
				isFunction = true;
				function = ProductFunc;
				if (isDigit( *src))
				{
					dim = UNSIGNED( src);
				}
				else
				{
					dim = 1;
				}
			}
			else if (isEqual( functionName, "asc"))
			{
				isFunction = true;
				function = AscendingFunc;
				if (isDigit( *src))
				{
					dim = UNSIGNED( src);
				}
				else
				{
					dim = 2;
				}
			}
			else if (isEqual( functionName, "permute"))
			{
				isFunction = true;
				function = PermutationFunc;
				if (isDigit( *src))
				{
					dim = UNSIGNED( src);
				}
				else
				{
					dim = 2;
				}
			}
			else if (isEqual( functionName, "join"))
			{
				isFunction = true;
				function = JoinFunc;
				if (isDigit( *src))
				{
					dim = UNSIGNED( src);
				}
				else
				{
					dim = 2;
				}
			}
			if (isFunction)
			{
				if (isColon( *src))
				{
					OPERATOR( src);
				}
				else
				{
					src = src_bk;
				}
			}
			else
			{
				if (isColon( *src))
				{
					throw std::runtime_error( std::string( "unknown function name '") + functionName + "'");
				}
				else
				{
					src = src_bk;
				}
			}
		}
		if (!*src)
		{
			throw std::runtime_error( "unexpected end of query in argument tuple set builder expression");
		}
		else if (isCloseSquareBracket(*src))
		{
			switch (function)
			{
				case ProductFunc:
				{
					if (selset.empty()) return SelectorSetR();
					while (dim > selset.size())
					{
						selset.push_back( selset.back());
					}
					return calcSelectorSet( TupleGenerator::Product, selset);
				}
				case AscendingFunc:
				{
					if (selset.empty()) return SelectorSetR();
					while (dim > selset.size())
					{
						selset.push_back( selset.back());
					}
					return calcSelectorSet( TupleGenerator::Ascending, selset);
				}
				case PermutationFunc:
				{
					if (selset.empty()) return SelectorSetR();
					while (dim > selset.size())
					{
						selset.push_back( selset.back());
					}
					return calcSelectorSet( TupleGenerator::Permutation, selset);
				}
				case JoinFunc:
				{
					if (selset.empty()) return SelectorSetR();
					SelectorSetR rt( new SelectorSet( selset.back()->rowsize()));
					std::vector<SelectorSetR>::const_iterator si = selset.begin(), se = selset.end();
					for (; si != se; ++si)
					{
						rt->append( **si);
					}
					return rt;
				}
			}
		}
		else if (isOpenSquareBracket(*src))
		{
			OPERATOR( src);
			selset.push_back( SELECTORSET( src, setmap));
		}
		else if (isAlpha(*src))
		{
			std::string setname( IDENTIFIER( src));
			strus::KeyMap<QueryParser::SetAttributes>::iterator si = setmap.find( setname);
			if (si == setmap.end())
			{
				selset.push_back( SelectorSetR());
			}
			else
			{
				si->second.referenced = true;

				SelectorSetR elemreflist( new SelectorSet( 1));
				std::size_t ei=0, ee=si->second.nofElements;
				for (; ei!=ee; ++ee)
				{
					Selector row( si->second.id, ei);
					elemreflist->pushRow( &row);
				}
				selset.push_back( elemreflist);
			}
		}
		else
		{
			throw std::runtime_error( "set identifier or set tuple builder expression in square brackets '[' ']' expected");
		}
	}
}

static std::vector<QueryParser::AccumulateOperation::Argument> ACCUARGS( char const*& src, strus::KeyMap<QueryParser::SetAttributes>& setmap, strus::KeyMap<unsigned int>& accumap)
{
	typedef QueryParser::AccumulateOperation::Argument Argument;
	std::vector<Argument> rt;
	if (isCloseOvalBracket(*src))
	{
		OPERATOR(src);
		return rt;
	}
	for (;;)
	{
		if (!isAlpha( *src))
		{
			throw std::runtime_error( "accumulator identifier expected");
		}
		std::string opname;
		std::string argname( IDENTIFIER( src));
		std::size_t argid = 0;
		double weight = 1.0;

		if (isOpenAngleBracket(*src))
		{
			// ACCUOP Like: ff<stemmed> 
			OPERATOR(src);
			if (!isAlpha( *src)) throw std::runtime_error("expected identifier (occurrence set) in angle brackets as operand of an accumulate operation");
			std::string opname = argname;
			std::string argname( IDENTIFIER( src));
			strus::KeyMap<QueryParser::SetAttributes>::iterator si = setmap.find( argname);
			if (si != setmap.end())
			{
				si->second.referenced = true;
				argid = si->second.id;
			}
			if (!isCloseAngleBracket(*src)) throw std::runtime_error("expected close angle bracket to close declaration of occurrency accumulator");
			OPERATOR(src);
		}
		else
		{
			// ACCUNAME Like: okapi
			strus::KeyMap<unsigned int>::iterator si = accumap.find( argname);
			if (si == accumap.end()) throw std::runtime_error( std::string("no accumulator found with name '") + argname + "'");
			argid = si->second;
		}
		if (isAsterisk( *src))
		{
			OPERATOR( src);
			if (isDigit( *src))
			{
				weight = DOUBLE( src);
			}
			else
			{
				throw std::runtime_error("floating point number expected as weight after '*'");
			}
		}
		if (argid != 0)
		{
			rt.push_back( Argument( opname, argid, weight));
		}
		if (isComma( *src))
		{
			OPERATOR(src);
		}
		else if (isCloseOvalBracket(*src))
		{
			OPERATOR(src);
			return rt;
		}
		else
		{
			throw std::runtime_error("comma ',' as argument separator or close oval bracket ')' to terminate argument list expected");
		}
	}
	return rt;
}


unsigned int QueryParser::defineSetElement( const std::string& setname, SetElement::Type type, std::size_t idx)
{
	std::size_t setdefidx;
	strus::KeyMap<SetAttributes>::iterator ti = m_setmap.find( setname);
	if (ti == m_setmap.end())
	{
		setdefidx = m_setdefs.size();
		m_setdefs.push_back( SetElementList());
		m_setmap[ setname] = SetAttributes( setdefidx+1, 1);
	}
	else
	{
		if (ti->second.referenced)
		{
			throw std::runtime_error( std::string( "try to add element to already referenced set '") + setname + "'");
		}
		setdefidx = ti->second.id-1;
		++ ti->second.nofElements;
	}
	m_setdefs[ setdefidx].push_back( SetElement( type, idx+1));
	return setdefidx+1;
}

void QueryParser::defineTerm( const std::string& setname, const std::string& type, const std::string& value)
{
	unsigned int resultsetIndex = defineSetElement( setname, SetElement::TermType, m_terms.size());
	Term term( resultsetIndex, type, value);
	m_terms.push_back( term);
}

void QueryParser::defineJoinOperation( const std::string& setname, const std::string& funcname, int range, const JoinOperation::SelectorSetR& input)
{
	unsigned int resultsetIndex = defineSetElement( setname, SetElement::IteratorType, m_joinOperations.size());
	JoinOperation op( resultsetIndex, funcname, range, input);
	m_joinOperations.push_back( op);
}

void QueryParser::defineAccumulateOperation( const std::string& accuname, const std::string& funcname, const std::vector<double>& scale, const std::vector<AccumulateOperation::Argument>& args)
{
	strus::KeyMap<unsigned int>::const_iterator ai = m_accumap.find( accuname);
	if (ai != m_accumap.end()) throw std::runtime_error( std::string( "duplicate definition of accumulator '") + accuname + "'");
	unsigned int accuIndex = m_accumulateOperations.size()+1;
	AccumulateOperation op( accuIndex, funcname, scale, args);
	m_accumulateOperations.push_back( op);
}

void QueryParser::pushQuery( const std::string& qry)
{
	char const* src = qry.c_str();
	try
	{
		skipSpaces( src);
		while (*src)
		{
			if (isAlpha(*src))
			{
				std::string resultname = IDENTIFIER( src);
				int range = 0;
				bool rangeSet = false;
				if (isColon(*src))
				{
					// PROD: IDENTIFIER<iteratorset> ':'
					//		IDENTIFIER<iteratorfunc>
					//		'[' builder ']' ';'
					OPERATOR( src);
					if (isAlpha( *src))
					{
						std::string opname( IDENTIFIER( src));
						std::vector<std::pair<std::string,int> > options;

						if (isDigit(*src))
						{
							range = INTEGER( src);
							rangeSet = true;
						}
						if (isOpenSquareBracket( *src))
						{
							//... assignment of an iterator from a join operation to a set
							OPERATOR( src);
							SelectorSetR input = SELECTORSET( src, m_setmap);
							defineJoinOperation( resultname, opname, range, input);
						}
						else if (rangeSet)
						{
							throw std::runtime_error( "range definition (integer after operation name) is only allowed for set selector expressions");
						}
						else
						{
							//... assignment of term to a set
							std::string value;
							if (isAlnum( *src))
							{
								value = IDENTIFIER( src);
							}
							else if (isStringQuote( *src))
							{
								value = STRING( src);
							}
							else
							{
								throw std::runtime_error( "unexpected token, string or alphanumeric sequence expected");
							}
							defineTerm( resultname, opname, value);
						}
					}
					else
					{
						throw std::runtime_error( "name of join function or term type expected after colon ':'");
					}
				}
				else if (isAssign(*src))
				{
					// PROD: IDENTIFIER<accumulatorset> '='
					//		IDENTIFIER<accumulatorfunc> weights
					//		'(' sets ')' ';'
					OPERATOR( src);
					if (isAlpha( *src))
					{
						std::string opname( IDENTIFIER( src));
						std::vector<double> scale;
	
						while (isDigit( *src))
						{
							scale.push_back( DOUBLE( src));
						}
						if (isOpenOvalBracket( *src))
						{
							OPERATOR(src);
							std::vector<AccumulateOperation::Argument> args = ACCUARGS( src, m_setmap, m_accumap);
							defineAccumulateOperation( resultname, opname, scale, args);
						}
						else
						{
							throw std::runtime_error( "oval brackets expected after accumulate function name and the scaling parameters");
						}
					}
					else
					{
						throw std::runtime_error( "name of accumulator function expected after assign '='");
					}
				}
			}
			if (*src)
			{
				if (isSemiColon( *src))
				{
					OPERATOR( src);
				}
				else
				{
					throw std::runtime_error( "semicolon ';' expected as query expression separator");
				}
			}
		}
	}
	catch (const std::runtime_error& e)
	{
		throw std::runtime_error(
			std::string( "error in query ")
			+ errorPosition( qry.c_str(), src)
			+ ":" + e.what());
	}
}




