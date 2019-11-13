/*
 * Copyright (c) 2018 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Various functions to instantiate strus storage and query evaluation components from configuration programs loaded from source
/// \file programLoader.cpp
#include "programLoader.hpp"
#include "strus/fileLocatorInterface.hpp"
#include "strus/queryProcessorInterface.hpp"
#include "strus/queryEvalInterface.hpp"
#include "strus/queryInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "strus/storageDocumentUpdateInterface.hpp"
#include "strus/storageTransactionInterface.hpp"
#include "strus/vectorStorageClientInterface.hpp"
#include "strus/vectorStorageTransactionInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/weightingFunctionInstanceInterface.hpp"
#include "strus/summarizerFunctionInstanceInterface.hpp"
#include "strus/scalarFunctionInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "strus/index.hpp"
#include "strus/base/stdint.h"
#include "strus/base/programLexer.hpp"
#include "strus/base/numstring.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/string_conv.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/dll_tags.hpp"
#include "strus/base/inputStream.hpp"
#include "strus/base/hton.hpp"
#include "private/internationalization.hpp"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <limits>
#include <iostream>
#include <sstream>

using namespace strus;

enum Tokens {
	TokIdentifier,
	TokFloat,
	TokInteger,
	TokOpenOvalBracket,
	TokCloseOvalBracket,
	TokOpenCurlyBracket,
	TokCloseCurlyBracket,
	TokOpenSquareBracket,
	TokCloseSquareBracket,
	TokOr,
	TokAssign,
	TokCompareNotEqual,
	TokCompareEqual,
	TokCompareGreaterEqual,
	TokCompareGreater,
	TokCompareLessEqual,
	TokCompareLess,
	TokDot,
	TokComma,
	TokColon,
	TokSemiColon,
	TokTilde,
	TokExp,
	TokAsterisk,
	TokLeftArrow,
	TokPath
};
static const char* g_tokens[] = {
	"[a-zA-Z_][a-zA-Z0-9_]*",
	"[+-]*[0-9][0-9_]*[.][0-9]*",
	"[+-]*[0-9][0-9_]*",
	"\\(",
	"\\)",
	"\\{",
	"\\}",
	"\\[",
	"\\]",
	"\\|",
	"\\=",
	"\\!\\=",
	"\\=\\=",
	"\\>\\=",
	"\\>",
	"\\<\\=",
	"\\<",
	"[.]",
	"[,]",
	"[:]",
	";",
	"\\~",
	"\\^",
	"\\*",
	"<-",
	"[/][^;,{} ]*",
	NULL
};
static const char* g_token_names[] = {
	"identifier",
	"flating point number",
	"integer",
	"open oval bracket '('",
	"close oval bracket ')'",
	"open curly bracket '{'",
	"close curly bracket '}'",
	"open square bracket '['",
	"close square bracket ']'",
	"or operator '|'",
	"assign '='",
	"not equl (\\!\\=)",
	"equality comparis operator \\=\\=",
	"greater equal comparis operator \\>\\=",
	"greater comparis operator \\>",
	"lesser equal comparin operator \\<\\=",
	"lesser equal comparison operator \\<",
	"dot '.'",
	"comma ','",
	"colon ':'",
	"semicolon ';'",
	"tilde '^",
	"exponent '^",
	"asterisk';'",
	"left arrow '<-'",
	"path",
	NULL
};
static const char* g_errtokens[] = {
	"[0-9][0-9]*[a-zA-Z_]",
	NULL
};
static const char* g_eolncomment = "#";

static const char* tokenName( const ProgramLexem& cur)
{
	switch (cur.type())
	{
		case ProgramLexem::Eof:		return "EOF";
		case ProgramLexem::SQString:	return "string";
		case ProgramLexem::DQString:	return "string";
		case ProgramLexem::Error:	return "bad lexem";
		case ProgramLexem::Token:	return g_token_names[ cur.id()];
	}
	return "?";
}

static void reportErrorWithLocation( ErrorBufferInterface* errorhnd, ProgramLexer& lexer, const char* msg, const char* what)
{
	try
	{
		std::string errorlocation = lexer.currentLocationString( -30, 80, "<!>");
		std::string errormsg;
		if (what)
		{
			errormsg = strus::string_format(
				_TXT("error in source on line %d (at %s): %s: %s"),
				(int)lexer.lineno(), errorlocation.c_str(), msg, what);
		}
		else
		{
			errormsg = strus::string_format(
				_TXT("error in source on line %d (at %s): %s"),
				(int)lexer.lineno(), errorlocation.c_str(), msg);
		}
		errorhnd->report( ErrorCodeSyntax, "%s", errormsg.c_str());
	}
	catch (const std::bad_alloc&)
	{
		errorhnd->report( ErrorCodeOutOfMem, _TXT("%s: out of memory handling error"), msg);
	}
}

static void parseTermConfig(
		QueryEvalInterface& qeval,
		ProgramLexer& lexer)
{
	if (lexer.current().isToken( TokIdentifier))
	{
		std::string termset = string_conv::tolower( lexer.current().value());
		lexer.next();
		if (!lexer.current().isToken( TokIdentifier) && !lexer.current().isString() && !lexer.current().isToken(TokInteger) && !lexer.current().isToken(TokFloat))
		{
			throw std::runtime_error( _TXT( "term value (string,identifier,number) after the feature identifier"));
		}
		std::string termvalue = lexer.current().value();
		lexer.next();

		if (!lexer.current().isToken( TokColon))
		{
			throw std::runtime_error( _TXT( "colon (':') expected after term value"));
		}
		lexer.next();

		if (!lexer.current().isToken( TokIdentifier))
		{
			throw std::runtime_error( _TXT( "term type identifier expected after colon and term value"));
		}
		std::string termtype = string_conv::tolower( lexer.current().value());
		lexer.next();

		qeval.addTerm( termset, termtype, termvalue);
	}
	else
	{
		throw std::runtime_error( _TXT( "feature set identifier expected as start of a term declaration in the query"));
	}
}

static NumericVariant parseNumericValue( ProgramLexer& lexer)
{
	NumericVariant rt;
	if (lexer.current().isToken(TokInteger))
	{
		rt = NumericVariant( numstring_conv::toint( lexer.current().value(), std::numeric_limits<int64_t>::max()));
	}
	else if (lexer.current().isToken(TokFloat))
	{
		rt = NumericVariant( numstring_conv::todouble( lexer.current().value()));
	}
	else
	{
		throw std::runtime_error( _TXT( "numeric value expected"));
	}
	lexer.next();
	return rt;
}

static void parseWeightingFormula(
		QueryEvalInterface& qeval,
		const QueryProcessorInterface* queryproc,
		ProgramLexer& lexer)
{
	std::string langName;
	if (lexer.current().isToken(TokIdentifier))
	{
		langName = string_conv::tolower( lexer.current().value());
		lexer.next();
	}
	if (!lexer.current().isString())
	{
		throw std::runtime_error( _TXT( "weighting formula string expected"));
	}
	std::string funcsrc = lexer.current().value();
	lexer.next();

	const ScalarFunctionParserInterface* scalarfuncparser = queryproc->getScalarFunctionParser(langName);
	strus::local_ptr<ScalarFunctionInterface> scalarfunc( scalarfuncparser->createFunction( funcsrc, std::vector<std::string>()));
	if (!scalarfunc.get())
	{
		throw std::runtime_error( _TXT( "failed to create scalar function (weighting formula) from source"));
	}
	qeval.defineWeightingFormula( scalarfunc.get());
	scalarfunc.release();
}

static void parseWeightingConfig(
		QueryEvalInterface& qeval,
		const std::string& weightingFeatureSet,
		const QueryProcessorInterface* queryproc,
		ProgramLexer& lexer)
{
	if (!lexer.current().isToken(TokIdentifier))
	{
		throw std::runtime_error( _TXT( "weighting function identifier expected"));
	}
	std::string functionName = string_conv::tolower( lexer.current().value());
	lexer.next();

	const WeightingFunctionInterface* wf = queryproc->getWeightingFunction( functionName);
	if (!wf) throw strus::runtime_error(_TXT( "weighting function '%s' not defined"), functionName.c_str());

	strus::local_ptr<WeightingFunctionInstanceInterface> function( wf->createInstance( queryproc));
	if (!function.get()) throw strus::runtime_error(_TXT( "failed to create weighting function '%s'"), functionName.c_str());

	typedef QueryEvalInterface::FeatureParameter FeatureParameter;
	std::vector<FeatureParameter> featureParameters;

	if (!lexer.current().isToken(TokOpenOvalBracket))
	{
		throw std::runtime_error( _TXT( "open oval bracket '(' expected after weighting function identifier"));
	}
	lexer.next();
	std::string debuginfoName;

	if (!lexer.current().isToken(TokCloseOvalBracket)) for (;;)
	{
		bool isFeatureParam = false;
		if (lexer.current().isToken(TokDot))
		{
			lexer.next();
			isFeatureParam = true;
		}
		if (!lexer.current().isToken(TokIdentifier))
		{
			throw std::runtime_error( _TXT( "identifier as start of parameter declaration (assignment parameter name to parameter value) expected"));
		}
		std::string parameterName = string_conv::tolower( lexer.current().value());
		lexer.next();
		if (!lexer.current().isToken(TokAssign))
		{
			throw std::runtime_error( _TXT( "assingment operator '=' expected after weighting function parameter name"));
		}
		lexer.next();

		if (!isFeatureParam && strus::caseInsensitiveEquals( parameterName, "debug"))
		{
			if (!debuginfoName.empty())
			{
				throw std::runtime_error( _TXT("duplicate definition of 'debug' parameter"));
			}
			if (lexer.current().isString() || lexer.current().isToken( TokIdentifier))
			{
				debuginfoName = string_conv::tolower( lexer.current().value());
				lexer.next();
			}
			else
			{
				throw std::runtime_error( _TXT("identifier or string expected as argument of 'debug' parameter"));
			}
		}
		else if (lexer.current().isToken(TokInteger) || lexer.current().isToken(TokFloat))
		{
			if (isFeatureParam)
			{
				throw std::runtime_error( _TXT( "feature parameter argument must be an identifier or string and not a number"));
			}
			function->addNumericParameter( parameterName, parseNumericValue( lexer));
		}
		else if (lexer.current().isString())
		{
			std::string parameterValue = lexer.current().value();
			lexer.next();
			if (isFeatureParam)
			{
				if (strus::caseInsensitiveEquals( weightingFeatureSet, parameterValue))
				{
					featureParameters.insert( featureParameters.begin(), FeatureParameter( parameterName, parameterValue));
				}
				else
				{
					featureParameters.push_back( FeatureParameter( parameterName, parameterValue));
				}
			}
			else
			{
				function->addStringParameter( parameterName, parameterValue);
			}
		}
		else if (lexer.current().isToken(TokIdentifier))
		{
			std::string parameterValue = lexer.current().value();
			lexer.next();
			if (isFeatureParam)
			{
				if (strus::caseInsensitiveEquals( weightingFeatureSet, parameterValue))
				{
					featureParameters.insert( featureParameters.begin(), FeatureParameter( parameterName, parameterValue));
				}
				else
				{
					featureParameters.push_back( FeatureParameter( parameterName, parameterValue));
				}
			}
			else
			{
				function->addStringParameter( parameterName, parameterValue);
			}
		}
		else
		{
			throw std::runtime_error( _TXT("parameter value (identifier,string,number) expected"));
		}
		if (!lexer.current().isToken(TokComma))
		{
			break;
		}
		lexer.next();
	}
	if (!lexer.current().isToken(TokCloseOvalBracket))
	{
		throw std::runtime_error( _TXT( "close oval bracket ')' expected at end of weighting function parameter list"));
	}
	lexer.next();
	qeval.addWeightingFunction( function.get(), featureParameters, debuginfoName); 
	(void)function.release();
}


static void parseSummarizerConfig(
		QueryEvalInterface& qeval,
		const QueryProcessorInterface* queryproc,
		ProgramLexer& lexer)
{
	std::string functionName;
	typedef QueryEvalInterface::FeatureParameter FeatureParameter;
	std::vector<FeatureParameter> featureParameters;

	if (!lexer.current().isToken(TokIdentifier))
	{
		throw std::runtime_error( _TXT( "name or id of summarizer expected at start of summarizer definition"));
	}
	std::string summaryId = string_conv::tolower( lexer.current().value());
	lexer.next();

	if (lexer.current().isToken(TokAssign))
	{
		lexer.next();
		if (!lexer.current().isToken(TokIdentifier))
		{
			throw std::runtime_error( _TXT( "name of summarizer function expected after assign operator '='"));
		}
		functionName = string_conv::tolower( lexer.current().value());
		lexer.next();
	}
	else
	{
		functionName = summaryId;
	}
	const SummarizerFunctionInterface* sf = queryproc->getSummarizerFunction( functionName);
	if (!sf) throw strus::runtime_error(_TXT( "summarizer function not defined: '%s'"), functionName.c_str());

	strus::local_ptr<SummarizerFunctionInstanceInterface> function( sf->createInstance( queryproc));
	if (!function.get()) throw strus::runtime_error(_TXT( "failed to create summarizer function instance '%s'"), functionName.c_str());

	if (!lexer.current().isToken(TokOpenOvalBracket))
	{
		throw std::runtime_error( _TXT( "open oval bracket '(' expected after summarizer function identifier"));
	}
	lexer.next();
	std::string debuginfoName;

	if (!lexer.current().isToken(TokCloseOvalBracket)) for (;;)
	{
		bool isFeatureParam = false;
		if (lexer.current().isToken(TokDot))
		{
			lexer.next();
			isFeatureParam = true;
		}
		if (!lexer.current().isToken(TokIdentifier))
		{
			throw std::runtime_error( _TXT( "identifier as start of parameter declaration (assignment parameter name to parameter value) expected"));
		}
		std::string parameterName = string_conv::tolower( lexer.current().value());
		lexer.next();
		if (!lexer.current().isToken(TokAssign))
		{
			throw std::runtime_error( _TXT( "assignment operator '=' expected after summarizer function parameter name"));
		}
		lexer.next();
		if (!isFeatureParam && strus::caseInsensitiveEquals( parameterName, "debug"))
		{
			if (!debuginfoName.empty())
			{
				throw std::runtime_error( _TXT("duplicate definition of 'debug' parameter"));
			}
			if (lexer.current().isToken( TokIdentifier) || lexer.current().isString())
			{
				debuginfoName = lexer.current().value();
				lexer.next();
			}
			else
			{
				throw std::runtime_error( _TXT("identifier or string expected as argument of 'debug' parameter"));
			}
		}
		else if (lexer.current().isToken( TokInteger) || lexer.current().isToken( TokFloat))
		{
			if (isFeatureParam)
			{
				throw std::runtime_error( _TXT( "feature parameter argument must be an identifier or string and not a number"));
			}
			function->addNumericParameter( parameterName, parseNumericValue( lexer));
		}
		else if (lexer.current().isString())
		{
			std::string parameterValue = lexer.current().value();
			lexer.next();
			if (isFeatureParam)
			{
				featureParameters.push_back( FeatureParameter( parameterName, parameterValue));
			}
			else
			{
				function->addStringParameter( parameterName, parameterValue);
			}
		}
		else if (lexer.current().isToken(TokIdentifier))
		{
			std::string parameterValue = lexer.current().value();
			lexer.next();
			if (isFeatureParam)
			{
				featureParameters.push_back( FeatureParameter( parameterName, parameterValue));
			}
			else
			{
				function->addStringParameter( parameterName, parameterValue);
			}
		}
		else
		{
			throw strus::runtime_error( _TXT( "unexpected token (%s) in summarizer function parameter list"), tokenName( lexer.current()));
		}
		if (!lexer.current().isToken(TokComma))
		{
			break;
		}
		lexer.next();
	}
	if (!lexer.current().isToken(TokCloseOvalBracket))
	{
		throw std::runtime_error( _TXT( "close oval bracket ')' expected at end of summarizer function parameter list"));
	}
	qeval.addSummarizerFunction( summaryId, function.get(), featureParameters, debuginfoName);
	(void)function.release();
	lexer.next();
}

DLL_PUBLIC bool strus::loadQueryEvalProgram(
		QueryEvalInterface& qeval,
		const std::vector<std::string>& analyzerterms,
		const QueryProcessorInterface* queryproc,
		const std::string& source,
		ErrorBufferInterface* errorhnd)
{
	enum StatementKeyword {e_FORMULA, e_EVAL, e_SELECT, e_WEIGHT, e_RESTRICTION, e_TERM, e_SUMMARIZE};
	std::string id;
	ProgramLexer lexer( source.c_str(), g_eolncomment, g_tokens, g_errtokens, errorhnd);

	try
	{
		std::string selectionFeatureSet;
		std::string weightingFeatureSet;

		lexer.next();

		while (!lexer.current().isEof())
		{
			if (!lexer.current().isToken(TokIdentifier))
			{
				throw std::runtime_error( _TXT( "expected identifier (keyword) starting a query eval program instruction"));
			}
			int kw = lexer.current().checkKeyword( 7, "FORMULA", "EVAL", "SELECT", "WEIGHT", "RESTRICT", "TERM", "SUMMARIZE");
			if (kw < 0)
			{
				throw strus::runtime_error( _TXT( "expected one of keywords %s"), "FORMULA, EVAL, SELECT, WEIGHT, RESTRICT, TERM, SUMMARIZE");
			}
			lexer.next();

			switch ((StatementKeyword)kw)
			{
				case e_TERM:
					parseTermConfig( qeval, lexer);
					break;
				case e_SELECT:
				{
					if (!selectionFeatureSet.empty())
					{
						throw std::runtime_error( _TXT("cannot handle more than one SELECT feature definition yet"));
					}
					if (!lexer.current().isToken(TokIdentifier))
					{
						throw std::runtime_error( _TXT( "expected identifier after SELECT"));
					}
					selectionFeatureSet = string_conv::tolower( lexer.current().value());
					lexer.next();
					qeval.addSelectionFeature( selectionFeatureSet);
					break;
				}
				case e_WEIGHT:
				{
					if (!weightingFeatureSet.empty())
					{
						throw std::runtime_error( _TXT("cannot handle more than one WEIGHT feature definition yet"));
					}
					if (!lexer.current().isToken(TokIdentifier))
					{
						throw std::runtime_error( _TXT( "expected identifier after WEIGHT"));
					}
					weightingFeatureSet = string_conv::tolower( lexer.current().value());
					lexer.next();
					break;
				}
				case e_RESTRICTION:
					if (!lexer.current().isToken(TokIdentifier))
					{
						throw std::runtime_error( _TXT( "expected identifier after RESTRICTION"));
					}
					while (lexer.current().isToken(TokIdentifier))
					{
						qeval.addRestrictionFeature( string_conv::tolower( lexer.current().value()));
						lexer.next();
						if (lexer.current().isToken(TokComma))
						{
							lexer.next();
						}
						else
						{
							break;
						}
					}
					break;
				case e_EVAL:
					if (weightingFeatureSet.empty())
					{
						throw std::runtime_error( _TXT( "WEIGHT (weighting feature not defined before EVAL)"));
					}
					parseWeightingConfig( qeval, weightingFeatureSet, queryproc, lexer);
					break;
				case e_FORMULA:
					parseWeightingFormula( qeval, queryproc, lexer);
					break;
				case e_SUMMARIZE:
					parseSummarizerConfig( qeval, queryproc, lexer);
					break;
			}
			if (!lexer.current().isToken( TokSemiColon))
			{
				throw std::runtime_error( _TXT( "semicolon expected as delimiter of query eval program instructions"));
			}
			lexer.next();
		}
		if (selectionFeatureSet.empty())
		{
			if (!analyzerterms.empty())
			{
				selectionFeatureSet = analyzerterms[ 0];
			}
			else
			{
				throw std::runtime_error( _TXT("no selection feature set (SELECT) defined in query evaluation configuration"));
			}
		}
		if (weightingFeatureSet.empty())
		{
			if (!analyzerterms.empty())
			{
				selectionFeatureSet = analyzerterms[ 0];
			}
			else
			{
				weightingFeatureSet = selectionFeatureSet;
			}
		}
		return true;
	}
	catch (const std::bad_alloc&)
	{
		reportErrorWithLocation( errorhnd, lexer, _TXT("out or memory loading query evaluation program"), 0);
		return false;
	}
	catch (const std::runtime_error& e)
	{
		reportErrorWithLocation( errorhnd, lexer, _TXT("error loading query evaluation program"), e.what());
		return false;
	}
}

static bool skipSpaces( char const*& itr)
{
	for (; *itr && (unsigned char)*itr <= 32; ++itr){}
	return *itr;
}

static bool skipNonSpaces( char const*& itr)
{
	for (; *itr && (unsigned char)*itr > 32; ++itr){}
	return *itr;
}

static bool skipToEoln( char const*& itr)
{
	for (; *itr && *itr != '\n'; ++itr){}
	return *itr;
}

static std::string parseNextItem( char const*& itr)
{
	std::string rt;
	if (!skipSpaces(itr))
	{
		throw std::runtime_error(_TXT("unexpected end of item"));
	}
	if (*itr >= '0' && *itr <= '9')
	{
		const char* start = itr;
		for (++itr; *itr >= '0' && *itr <= '9'; ++itr){}
		if (!*itr || (unsigned char)*itr <= 32)
		{
			rt = std::string( start, itr-start);
			return rt;
		}
		else
		{
			itr = start;
		}
	}
	if (*itr == '"' || *itr == '\'')
	{
		char eb = *itr++;
		for (++itr; *itr && *itr != eb; ++itr)
		{
			if (*itr=='\\')
			{
				++itr;
			}
			rt.push_back( *itr);
		}
		if (!*itr) throw std::runtime_error(_TXT("string not terminated"));
	}
	else
	{
		const char* start = itr;
		for (; *itr && (unsigned char)*itr > 32; ++itr){}
		rt.append( start, itr-start);
	}
	return rt;
}

static strus::Index parseDocno( StorageClientInterface& storage, char const*& itr)
{
	strus::Index rt = 0;
	std::string docid( parseNextItem( itr));
	if (docid.empty())
	{
		throw std::runtime_error(_TXT("document id is empty"));
	}
	if (docid[0] >= '0' && docid[0] <= '9')
	{
		rt = numstring_conv::toint( docid, std::numeric_limits<strus::Index>::max());
	}
	else if (!docid.empty() && docid[0] == '_')
	{
		rt = numstring_conv::toint( docid.c_str()+1, docid.size()-1, std::numeric_limits<strus::Index>::max());
	}
	else
	{
		rt = storage.documentNumber( docid);
	}
	return rt;
}

static unsigned int parseUnsigned( char const*& itr)
{
	std::string item( parseNextItem( itr));
	return numstring_conv::touint( item, std::numeric_limits<unsigned int>::max());
}

static void storeMetaDataValue( StorageTransactionInterface& transaction, const Index& docno, const std::string& name, const NumericVariant& val)
{
	strus::local_ptr<StorageDocumentUpdateInterface> update( transaction.createDocumentUpdate( docno));
	if (!update.get()) throw strus::runtime_error( _TXT("failed to create document update structure"));

	update->setMetaData( name, val);
	update->done();
}

static void storeAttributeValue( StorageTransactionInterface& transaction, const Index& docno, const std::string& name, const std::string& val)
{
	strus::local_ptr<StorageDocumentUpdateInterface> update( transaction.createDocumentUpdate( docno));
	if (!update.get()) throw strus::runtime_error( _TXT("failed to create document update structure"));
	if (val.empty())
	{
		update->clearAttribute( name);
	}
	else
	{
		update->setAttribute( name, val);
	}
	update->done();
}

static void storeUserRights( StorageTransactionInterface& transaction, const Index& docno, const std::string& val)
{
	strus::local_ptr<StorageDocumentUpdateInterface> update( transaction.createDocumentUpdate( docno));
	if (!update.get()) throw strus::runtime_error( _TXT("failed to create document update structure"));
	char const* itr = val.c_str();

	while (skipSpaces( itr))
	{
		bool positive = true;
		if (*itr == '-')
		{
			++itr;
			if (!skipSpaces(itr))
			{
				update->clearUserAccessRights();
				break;
			}
			else if (*itr == ',')
			{
				update->clearUserAccessRights();
				++itr;
				continue;
			}
			positive = false;
		}
		else if (*itr == '+')
		{
			positive = true;
			++itr;
			if (!skipSpaces(itr))
			{
				throw strus::runtime_error( _TXT("username expected after '+'"));
			}
		}
		std::string username = parseNextItem(itr);
		if (positive)
		{
			update->setUserAccessRight( username);
		}
		else
		{
			update->clearUserAccessRight( username);
		}
		if (*itr == ',')
		{
			++itr;
			continue;
		}
		else if (skipSpaces(itr))
		{
			throw strus::runtime_error( _TXT("unexpected token in user rigths specification"));
		}
	}
}


enum StorageValueType
{
	StorageValueMetaData,
	StorageValueAttribute,
	StorageUserRights
};

static bool updateStorageValue(
		StorageTransactionInterface* transaction,
		const Index& docno,
		const std::string& elementName,
		StorageValueType valueType,
		const std::string& value)
{
	switch (valueType)
	{
		case StorageValueMetaData:
		{
			NumericVariant val( value.c_str());
			storeMetaDataValue( *transaction, docno, elementName, val);
			return true;
		}
		case StorageValueAttribute:
		{
			storeAttributeValue( *transaction, docno, elementName, value);
			return true;
		}
		case StorageUserRights:
		{
			storeUserRights( *transaction, docno, value);
			return true;
		}
	}
	return false;
}

typedef std::multimap<std::string,strus::Index> KeyDocnoMap;
static unsigned int loadStorageValues(
		StorageClientInterface& storage,
		const std::string& elementName,
		const KeyDocnoMap* attributemapref,
		const std::string& file,
		StorageValueType valueType,
		unsigned int commitSize,
		ErrorBufferInterface* errorhnd)
{
	InputStream stream( file);
	if (stream.error()) throw strus::runtime_error(_TXT("failed to open storage value file '%s': %s"), file.c_str(), ::strerror(stream.error()));
	unsigned int rt = 0;
	strus::local_ptr<StorageTransactionInterface> transaction( storage.createTransaction());
	if (!transaction.get()) throw strus::runtime_error( _TXT("failed to create storage transaction"));
	std::size_t linecnt = 1;
	unsigned int commitcnt = 0;
	try
	{
		char line[ 4096];
		for (; stream.readLine( line, sizeof(line)); ++linecnt)
		{
			char const* itr = line;
			if (attributemapref)
			{
				std::string attr = parseNextItem( itr);
				std::pair<KeyDocnoMap::const_iterator,KeyDocnoMap::const_iterator>
					range = attributemapref->equal_range( attr);
				KeyDocnoMap::const_iterator ki = range.first, ke = range.second;
				for (; ki != ke; ++ki)
				{
					if (updateStorageValue( transaction.get(), ki->second, elementName, valueType, itr))
					{
						rt += 1;
					}
				}
			}
			else
			{
				Index docno = parseDocno( storage, itr);
				if (!docno)
				{
					if (errorhnd->hasError()) throw std::runtime_error(errorhnd->fetchError());
					continue;
				}
				if (updateStorageValue( transaction.get(), docno, elementName, valueType, itr))
				{
					rt += 1;
				}
			}
			if (++commitcnt == commitSize)
			{
				if (!transaction->commit())
				{
					throw std::runtime_error( _TXT("transaction commit failed"));
				}
				commitcnt = 0;
				transaction.reset( storage.createTransaction());
				if (!transaction.get()) throw strus::runtime_error( _TXT("failed to recreate storage transaction after commit"));
			}
		}
		if (stream.error())
		{
			throw strus::runtime_error(_TXT("failed to read from storage value file '%s': %s"), file.c_str(), ::strerror(stream.error()));
		}
		if (commitcnt)
		{
			if (!transaction->commit())
			{
				throw std::runtime_error( _TXT("transaction commit failed"));
			}
			commitcnt = 0;
			transaction.reset( storage.createTransaction());
			if (!transaction.get()) throw strus::runtime_error( _TXT("failed to recreate storage transaction after commit"));
		}
		return rt;
	}
	catch (const std::runtime_error& err)
	{
		throw strus::runtime_error( _TXT("error on line %u: %s"), (unsigned int)linecnt, err.what());
	}
}


DLL_PUBLIC int strus::loadDocumentMetaDataAssignments(
		StorageClientInterface& storage,
		const std::string& metadataName,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		int commitSize,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		return loadStorageValues( storage, metadataName, attributemapref, file, StorageValueMetaData, commitSize, errorhnd);
	}
	catch (const std::bad_alloc&)
	{
		errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory loading meta data assignments"));
		return 0;
	}
	catch (const std::runtime_error& e)
	{
		errorhnd->report( ErrorCodeRuntimeError, _TXT("error loading meta data assignments: %s"), e.what());
		return 0;
	}
}


DLL_PUBLIC int strus::loadDocumentAttributeAssignments(
		StorageClientInterface& storage,
		const std::string& attributeName,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		int commitSize,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		return loadStorageValues( storage, attributeName, attributemapref, file, StorageValueAttribute, commitSize, errorhnd);
	}
	catch (const std::bad_alloc&)
	{
		errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory loading attribute assignments"));
		return 0;
	}
	catch (const std::runtime_error& e)
	{
		errorhnd->report( ErrorCodeRuntimeError, _TXT("error loading attribute assignments: %s"), e.what());
		return 0;
	}
}


DLL_PUBLIC int strus::loadDocumentUserRightsAssignments(
		StorageClientInterface& storage,
		const std::multimap<std::string,strus::Index>* attributemapref,
		const std::string& file,
		int commitSize,
		ErrorBufferInterface* errorhnd)
{
	try
	{
		return loadStorageValues( storage, std::string(), attributemapref, file, StorageUserRights, commitSize, errorhnd);
	}
	catch (const std::bad_alloc&)
	{
		errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory loading user right assignments"));
		return 0;
	}
	catch (const std::runtime_error& e)
	{
		errorhnd->report( ErrorCodeRuntimeError, _TXT("error loading user right assignments: %s"), e.what());
		return 0;
	}
}

static void parseVectorFeatureType( char const*& type, std::size_t& typesize, char const*& term, std::size_t& termsize, const char* item, std::size_t itemsize, char typeValueSeparator)
{
	type = term = item;
	termsize = itemsize;
	typesize = 0;
	for (; typesize < termsize && type[ typesize] != typeValueSeparator; ++typesize){}
	if (typesize == termsize)
	{
		if ((termsize == 4 && 0==std::memcmp( term, "</s>", termsize))
		||  (termsize == 1 && (term[0] == '.' || term[0] == ',')))
		{
			typesize = 0;
			type = "";
		}
		else
		{
			throw strus::runtime_error( _TXT("value with not type but type/value separator specified"));
		}
	}
	else
	{
		term = type + typesize + 1;
		termsize -= typesize + 1;
	}
}

static void loadVectorStorageVectors_word2vecBin( 
		VectorStorageClientInterface* client,
		const std::string& vectorfile,
		bool networkOrder,
		char typeValueSeparator,
		int commitSize,
		ErrorBufferInterface* errorhnd)
{
	unsigned int linecnt = 0;
	try
	{
		DebugTraceContextInterface* debugtrace = 0;
		strus::local_ptr<DebugTraceContextInterface> debugtraceref;
		DebugTraceInterface* dbg = errorhnd->debugTrace();
		if (!dbg)
		{
			debugtraceref.reset( dbg->createTraceContext( "vector"));
			debugtrace = debugtraceref.get();
		}
		strus::local_ptr<VectorStorageTransactionInterface> transaction( client->createTransaction());
		if (!transaction.get()) throw std::runtime_error( _TXT("create transaction failed"));

		InputStream infile( vectorfile);
		if (infile.error())
		{
			throw strus::runtime_error(_TXT("failed to open word2vec file '%s': %s"), vectorfile.c_str(), ::strerror(infile.error()));
		}
		unsigned int collsize = 0;
		unsigned int vecsize = 0;
		std::size_t linesize;
		{
			// Read first text line, that contains two numbers, the collection size and the vector size:
			char firstline[ 256];
			linesize = infile.readAhead( firstline, sizeof(firstline)-1);
			firstline[ linesize] = '\0';
			char const* si = firstline;
			const char* se = std::strchr( si, '\n');
			if (!se) throw std::runtime_error( _TXT("failed to parse header line"));
			collsize = parseUnsigned( si);
			vecsize = parseUnsigned( si);
			if (*(si-1) != '\n')
			{
				if (skipToEoln( si)) ++si;
			}
			infile.read( firstline, si - firstline);
		}
		// Declare buffer for reading lines:
		struct charp_scope
		{
			charp_scope( char* ptr_)	:ptr(ptr_){}
			~charp_scope()			{if (ptr) std::free(ptr);}
			char* ptr;
		};
		int nofVectors = 0;
		int totalNofVectors = 0;
		if (commitSize == 0)
		{
			commitSize = std::numeric_limits<int>::max();
		}
		enum {MaxIdSize = 2048};
		std::size_t linebufsize = MaxIdSize + vecsize * sizeof(float);
		char* linebuf = (char*)std::malloc( linebufsize);
		charp_scope linebuf_scope( linebuf);

		// Parse vector by vector and add them to the transaction till EOF:
		linesize = infile.readAhead( linebuf, linebufsize);
		while (linesize)
		{
			++linecnt;
			char const* si = linebuf;
			const char* se = linebuf + linesize;
			for (; si < se && (unsigned char)*si > 32; ++si){}
			const char* term = linebuf;
			std::size_t termsize = si - linebuf;
			const char* type = linebuf;
			std::size_t typesize = 0;
			if (typeValueSeparator)
			{
				parseVectorFeatureType( type, typesize, term, termsize, term, termsize, typeValueSeparator);
			}
			++si;
			if (si+vecsize*sizeof(float) > se)
			{
				throw strus::runtime_error( _TXT("wrong file format"));
			}
			std::ostringstream vecstrbuf;
			std::vector<float> vec;
			vec.reserve( vecsize);
			unsigned int ii = 0;
			if (networkOrder)
			{
				for (; ii < vecsize; ii++)
				{
					float_net_t val;
					std::memcpy( (void*)&val, si, sizeof( val));
					vec.push_back( ByteOrder<float>::ntoh( val));
					si += sizeof( float);
				}
			}
			else
			{
				for (; ii < vecsize; ii++)
				{
					float val;
					std::memcpy( (void*)&val, si, sizeof( val));
					si += sizeof( float);
					vec.push_back( val);
				}
			}
			double len = 0;
			std::vector<float>::iterator vi = vec.begin(), ve = vec.end();
			for (; vi != ve; ++vi)
			{
				double vv = *vi;
				len += vv * vv;
			}
			len = sqrt( len);
			vi = vec.begin(), ve = vec.end();
			for (int vidx=0; vi != ve; ++vi,++vidx)
			{
				*vi /= len;
				if (*vi >= -1.0 && *vi <= 1.0)
				{
					if (debugtrace) vecstrbuf << (vidx?", ":"") << *vi;
				}
				else
				{
					throw strus::runtime_error( _TXT("illegal value in vector: %f %f"), *vi, len);
				}
			}
			transaction->defineVector( std::string( type, typesize), std::string( term, termsize), vec);
			if (debugtrace)
			{
				std::string termstr( term, termsize);
				std::string vecstr( vecstrbuf.str());
				debugtrace->event( "vecterm", "name '%s' vec %s", termstr.c_str(), vecstr.c_str());
			}
			if (errorhnd->hasError())
			{
				throw strus::runtime_error(_TXT("add vector failed: %s"), errorhnd->fetchError());
			}
			if (*si == '\n')
			{
				++si;
			}
			else
			{
				throw strus::runtime_error(_TXT("end of line marker expected after binary vector instead of '%x'"), (unsigned int)(unsigned char)*si);
			}
			++totalNofVectors;
			if (++nofVectors >= commitSize)
			{
				nofVectors = 0;
				if (!transaction->commit())
				{
					throw strus::runtime_error(_TXT("vector storage transaction failed: %s"), errorhnd->fetchError());
				}
				transaction.reset( client->createTransaction());
				if (!transaction.get()) throw std::runtime_error( _TXT("create transaction failed"));
				if (debugtrace)
				{
					if (debugtrace) debugtrace->event( "commit", "inserted vectors %d", totalNofVectors);
				}
			}
			infile.read( linebuf, si - linebuf);
			linesize = infile.readAhead( linebuf, linebufsize);
		}
		if (infile.error())
		{
			throw strus::runtime_error(_TXT("failed to read from word2vec file '%s': %s"), vectorfile.c_str(), ::strerror(infile.error()));
		}
		if (collsize != linecnt)
		{
			throw std::runtime_error( _TXT("collection size does not match"));
		}
		if (nofVectors)
		{
			if (!transaction->commit())
			{
				throw strus::runtime_error(_TXT("vector storage transaction failed: %s"), errorhnd->fetchError());
			}
			if (debugtrace)
			{
				if (debugtrace) debugtrace->event( "commit", "inserted vectors %d", totalNofVectors);
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		throw strus::runtime_error( _TXT("in word2vec binary file in record %u: %s"), linecnt, err.what());
	}
}

static void loadVectorStorageVectors_word2vecText( 
		VectorStorageClientInterface* client,
		const std::string& vectorfile,
		char typeValueSeparator,
		int commitSize,
		ErrorBufferInterface* errorhnd)
{
	unsigned int linecnt = 0;
	try
	{
		DebugTraceContextInterface* debugtrace = 0;
		strus::local_ptr<DebugTraceContextInterface> debugtraceref;
		DebugTraceInterface* dbg = errorhnd->debugTrace();
		if (!dbg)
		{
			debugtraceref.reset( dbg->createTraceContext( "vector"));
			debugtrace = debugtraceref.get();
		}
		strus::local_ptr<VectorStorageTransactionInterface> transaction( client->createTransaction());
		if (!transaction.get()) throw std::runtime_error( _TXT("create transaction failed"));
		InputStream infile( vectorfile);
		if (infile.error())
		{
			throw strus::runtime_error(_TXT("failed to open word2vec file '%s': %s"), vectorfile.c_str(), ::strerror(infile.error()));
		}
		enum {LineBufSize=1<<20};
		struct charp_scope
		{
			charp_scope( char* ptr_)	:ptr(ptr_){}
			~charp_scope()			{if (ptr) std::free(ptr);}
			char* ptr;
		};
		int nofVectors = 0;
		int totalNofVectors = 0;
		if (commitSize == 0)
		{
			commitSize = std::numeric_limits<int>::max();
		}
		char* linebuf = (char*)std::malloc( LineBufSize);
		charp_scope linebuf_scope(linebuf);
		const char* line = infile.readLine( linebuf, LineBufSize);
		for (; line; line = infile.readLine( linebuf, LineBufSize))
		{
			char const* si = line;
			if (std::strlen(si) >= LineBufSize-1) throw std::runtime_error( _TXT("input line too long"));
			++linecnt;
			const char* term;
			std::size_t termsize;
			std::vector<float> vec;
			std::ostringstream vecstrbuf;
			if (!skipSpaces(si)) throw std::runtime_error( _TXT("unexpected end of line"));
			term = si;
			skipNonSpaces( si);
			if (si) throw std::runtime_error( _TXT("unexpected end of file"));
			termsize = si - term;
			const char* type = term;
			std::size_t typesize = 0;
			if (typeValueSeparator)
			{
				parseVectorFeatureType( type, typesize, term, termsize, term, termsize, typeValueSeparator);
			}
			++si;
			if (!skipSpaces(si)) throw std::runtime_error( _TXT("unexpected end of line"));
			while (*si)
			{
				char const* sn = si;
				skipNonSpaces(sn);
				vec.push_back( numstring_conv::todouble( si, sn-si));
				if (debugtrace) vecstrbuf << (vec.empty()?"":", ") << vec.back();
				si = sn;
				skipSpaces(si);
			}
			double len = 0;
			std::vector<float>::iterator vi = vec.begin(), ve = vec.end();
			for (; vi != ve; ++vi)
			{
				double vv = *vi;
				len += vv * vv;
			}
			len = sqrt( len);
			for (; vi != ve; vi++)
			{
				*vi /= len;
				if (*vi >= -1.0 && *vi <= 1.0)
				{/*OK*/}
				else
				{
					throw strus::runtime_error( _TXT("illegal value in vector: %f %f"), *vi, len);
				}
			}
			transaction->defineVector( std::string( type, typesize), std::string( term, termsize), vec);
			if (debugtrace)
			{
				std::string termstr( term, termsize);
				std::string vecstr( vecstrbuf.str());
				if (debugtrace) debugtrace->event( "vecterm", "name '%s' vec %s", termstr.c_str(), vecstr.c_str());
			}
			if (errorhnd->hasError())
			{
				throw strus::runtime_error(_TXT("add vector failed: %s"), errorhnd->fetchError());
			}
			++totalNofVectors;
			if (++nofVectors >= commitSize)
			{
				nofVectors = 0;
				if (!transaction->commit())
				{
					throw strus::runtime_error(_TXT("vector storage transaction failed: %s"), errorhnd->fetchError());
				}
				transaction.reset( client->createTransaction());
				if (!transaction.get()) throw std::runtime_error( _TXT("create transaction failed"));
				if (debugtrace)
				{
					if (debugtrace) debugtrace->event( "commit", "inserted vectors %d", totalNofVectors);
				}
			}
		}
		if (infile.error())
		{
			throw strus::runtime_error(_TXT("failed to read from word2vec file '%s': %s"), vectorfile.c_str(), ::strerror(infile.error()));
		}
		if (nofVectors)
		{
			if (!transaction->commit())
			{
				throw strus::runtime_error(_TXT("vector storage transaction failed: %s"), errorhnd->fetchError());
			}
			if (debugtrace)
			{
				if (debugtrace) debugtrace->event( "commit", "inserted vectors %d", totalNofVectors);
			}
		}
	}
	catch (const std::runtime_error& err)
	{
		throw strus::runtime_error( _TXT("in word2vec text file on line %u: %s"), linecnt, err.what());
	}
}

DLL_PUBLIC bool strus::loadVectorStorageVectors( 
		VectorStorageClientInterface* client,
		const std::string& vectorfile,
		bool networkOrder,
		char typeValueSeparator,
		int commitSize,
		ErrorBufferInterface* errorhnd)
{
	char const* filetype = 0;
	try
	{
		if (strus::isTextFile( vectorfile))
		{
			filetype = "word2vec text file";
			loadVectorStorageVectors_word2vecText( client, vectorfile, typeValueSeparator, commitSize, errorhnd);
		}
		else
		{
			filetype = "word2vec binary file";
			loadVectorStorageVectors_word2vecBin( client, vectorfile, networkOrder, typeValueSeparator, commitSize, errorhnd);
		}
		return true;
	}
	catch (const std::bad_alloc&)
	{
		errorhnd->report( ErrorCodeOutOfMem, _TXT("out of memory loading feature vectors from file %s (file format: %s)"), vectorfile.c_str(), filetype);
		return false;
	}
	catch (const std::runtime_error& e)
	{
		errorhnd->report( ErrorCodeRuntimeError, _TXT("error loading feature vectors from file %s (file format: %s): %s"), vectorfile.c_str(), filetype, e.what());
		return false;
	}
}


