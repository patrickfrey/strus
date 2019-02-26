/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the sentence analyzer detecting sentence candidates in queries
#include "sentenceAnalyzerInstance.hpp"
#include "strus/sentenceLexerInstanceInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "strus/base/string_format.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <string>

using namespace strus;

#define MODULENAME "sentence analyzer"
#define STRUS_DBGTRACE_COMPONENT_NAME "sentence"

SentenceAnalyzerInstance::SentenceAnalyzerInstance( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_),m_debugtrace(0),m_instructionar(),m_patternar(),m_valuear(),m_weightar(),m_addressar()
{
	DebugTraceInterface* dbgi = m_errorhnd->debugTrace();
	if (dbgi) m_debugtrace = dbgi->createTraceContext( STRUS_DBGTRACE_COMPONENT_NAME);
}

SentenceAnalyzerInstance::~SentenceAnalyzerInstance()
{
	if (m_debugtrace) delete m_debugtrace;
}

SentenceAnalyzerInstance::Pattern::Pattern( const std::string& str_, ErrorBufferInterface* errorhnd)
	:regex(new RegexSearch( str_, 0, errorhnd)),str(str_)
{
	if (!regex.get()) throw strus::runtime_error( _TXT("error in regular expression '%s': %s"), str.c_str(), errorhnd->fetchError());
}

SentenceAnalyzerInstance::Pattern::Pattern( const Pattern& o)
	:regex(o.regex),str(o.str){}


std::string SentenceAnalyzerInstance::instructionToString( const Instruction& instr) const
{
	std::string rt;
	const char* opstr = opCodeName( instr.opcode);
	rt.append( opstr);
	switch (instr.opcode)
	{
		case OpNop:
			break;
		case OpJmp:
		case OpJmpIf:
		case OpJmpIfNot:
		case OpStartCount:
			rt.append( strus::string_format( " %d", instr.arg));
			break;
		case OpEndCount:
		case OpDecCount:
			break;
		case OpTestType:
			rt.push_back(' ');
			rt.append( m_valuear[ instr.arg]);
			break;
		case OpTestFeat:
			rt.push_back(' ');
			rt.append( m_patternar[ instr.arg].str);
			break;
		case OpWeight:
			rt.append( strus::string_format( " %.3f", m_weightar[ instr.arg]));
			break;
		case OpMark:
		case OpReMark:
		case OpRelease:
		case OpAccept:
		case OpReject:
			break;
		case OpResult:
			rt.push_back(' ');
			rt.append( m_valuear[ instr.arg]);
			break;
	}
	return rt;
}

void SentenceAnalyzerInstance::printInstructions( std::ostream& out, int fromAddr, int toAddr) const
{
	if (toAddr < 0) toAddr = m_instructionar.size();
	if (fromAddr < 0) fromAddr = 0;
	for (int ip=fromAddr; ip<toAddr; ++ip)
	{
		out << instructionToString( m_instructionar[ ip]) << std::endl;
	}
}

void SentenceAnalyzerInstance::pushInstructionInt( OpCode opcode, int arg)
{
	m_instructionar.push_back( Instruction( opcode, arg));
}

void SentenceAnalyzerInstance::pushInstructionFloat( OpCode opcode, float arg)
{
	m_weightar.push_back( arg);
	m_instructionar.push_back( Instruction( opcode, m_valuear.size()));
}

void SentenceAnalyzerInstance::pushInstructionRegex( OpCode opcode, const std::string& arg)
{
	m_patternar.push_back( Pattern( arg, m_errorhnd));
	m_instructionar.push_back( Instruction( opcode, m_valuear.size()));
}

void SentenceAnalyzerInstance::pushInstructionString( OpCode opcode, const std::string& arg)
{
	m_valuear.push_back( arg);
	m_instructionar.push_back( Instruction( opcode, m_valuear.size()));
}

void SentenceAnalyzerInstance::pushInstruction( OpCode opcode)
{
	m_instructionar.push_back( Instruction( opcode, 0));
}

void SentenceAnalyzerInstance::insertInstructionInt( int iaddr, OpCode opcode, int arg)
{
	patchOpCodeJmpOffset( iaddr, iaddr, +1);
	m_instructionar.insert( m_instructionar.begin() + iaddr, Instruction( opcode, arg));
}

void SentenceAnalyzerInstance::insertInstruction( int iaddr, OpCode opcode)
{
	patchOpCodeJmpOffset( iaddr, iaddr, +1);
	m_instructionar.insert( m_instructionar.begin() + iaddr, Instruction( opcode, 0));
}

bool SentenceAnalyzerInstance::isOpCodeJmp( OpCode opcode)
{
	return (opcode == OpJmp || opcode == OpJmpIf || opcode == OpJmpIfNot);
}

void SentenceAnalyzerInstance::patchOpCodeJmpAbsolute( int istart, int iend, int addr, int patchaddr)
{
	std::vector<Instruction>::iterator ii = m_instructionar.begin() + istart, ie = m_instructionar.begin() + iend;
	for (; ii != ie; ++ii)
	{
		if (isOpCodeJmp( ii->opcode) && ii->arg == addr) ii->arg = patchaddr;
	}
}

void SentenceAnalyzerInstance::patchOpCodeJmpOffset( int istart, int addr, int patchaddrincr)
{
	std::vector<Instruction>::iterator ii = m_instructionar.begin() + istart, ie = m_instructionar.end();
	for (; ii != ie; ++ii)
	{
		if (isOpCodeJmp( ii->opcode) && ii->arg >= addr) ii->arg += patchaddrincr;
	}
	std::vector<int>::iterator ai = m_addressar.begin(), ae = m_addressar.end();
	for (; ai != ae; ++ai)
	{
		if (*ai >= istart) *ai += patchaddrincr;
	}
}


#define ADDRESS_SUCCESS -1
#define ADDRESS_FAIL -2

void SentenceAnalyzerInstance::pushTerm( const std::string& type, const std::string& name, float weight)
{
	try
	{
		m_addressar.push_back( m_instructionar.size());
		pushInstructionString( OpTestType, type);
		pushInstructionRegex( OpTestFeat, name);
		pushInstructionInt( OpJmpIfNot, ADDRESS_FAIL);
		pushInstructionFloat( OpWeight, weight);
		pushInstruction( OpAccept);
		pushInstructionInt( OpJmp, ADDRESS_SUCCESS);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push term: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushAlt( int argc)
{
	try
	{
		if (argc > (int)m_addressar.size()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		if (argc == 0) throw std::runtime_error(_TXT("bad argument, no expressions referenced"));

		std::vector<int>::iterator ai = m_addressar.begin() + (m_addressar.size() - argc), ae = m_addressar.end();
		insertInstruction( *ai, OpMark);
		std::vector<int>::iterator ai_next = ai + 1;
		for (; ai_next != ae; ++ai,++ai_next)
		{
			insertInstruction( *ai_next, OpReMark);
			patchOpCodeJmpAbsolute( *ai, *ai_next, ADDRESS_FAIL, *ai_next);
			patchOpCodeJmpAbsolute( *ai, *ai_next, ADDRESS_SUCCESS, m_instructionar.size()+2);
		}
		patchOpCodeJmpAbsolute( *ai, m_instructionar.size(), ADDRESS_FAIL, m_instructionar.size());
		patchOpCodeJmpAbsolute( *ai, m_instructionar.size(), ADDRESS_SUCCESS, m_instructionar.size()+2);
		pushInstruction( OpReject);
		pushInstructionInt( OpJmp, ADDRESS_FAIL);
		pushInstruction( OpRelease);
		pushInstructionInt( OpJmp, ADDRESS_SUCCESS);
		m_addressar.resize( m_addressar.size() - argc + 1);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push alt: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushSequenceImm( int argc)
{
	try
	{
		if (argc > (int)m_addressar.size()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		if (argc == 0) throw std::runtime_error(_TXT("bad argument, no expressions referenced"));

		std::vector<int>::iterator ai = m_addressar.begin() + (m_addressar.size() - argc), ae = m_addressar.end();
		insertInstruction( *ai, OpMark);
		std::vector<int>::iterator ai_next = m_addressar.begin() + 1;
		for (; ai_next != ae; ++ai,++ai_next)
		{
			patchOpCodeJmpAbsolute( *ai, *ai_next, ADDRESS_SUCCESS, *ai_next);
			patchOpCodeJmpAbsolute( *ai, m_instructionar.size(), ADDRESS_FAIL, m_instructionar.size()+2);
		}
		patchOpCodeJmpAbsolute( *ai, m_instructionar.size(), ADDRESS_SUCCESS, m_instructionar.size());
		patchOpCodeJmpAbsolute( *ai, m_instructionar.size(), ADDRESS_FAIL, m_instructionar.size()+2);
		pushInstruction( OpRelease);
		pushInstructionInt( OpJmp, ADDRESS_SUCCESS);
		pushInstruction( OpReject);
		pushInstructionInt( OpJmp, ADDRESS_FAIL);
		m_addressar.resize( m_addressar.size() - argc + 1);
		
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push sequence immediate: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushRepeat( int times)
{
	try
	{
		if (m_addressar.empty()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		int addr = m_addressar.back();

		insertInstructionInt( addr++, OpStartCount, times);
		insertInstruction( addr++, OpMark);

		patchOpCodeJmpAbsolute( addr, m_instructionar.size(), ADDRESS_SUCCESS, m_instructionar.size());
		patchOpCodeJmpAbsolute( addr, m_instructionar.size(), ADDRESS_FAIL, m_instructionar.size()+2);

		pushInstruction( OpDecCount);
		pushInstructionInt( OpJmpIf, addr/*address of OpMark*/);
		pushInstruction( OpEndCount);
		pushInstruction( OpRelease);
		pushInstructionInt( OpJmp, ADDRESS_SUCCESS);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push repeat: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::defineSentence( const std::string& classname, float weight)
{
	try
	{
		if (m_addressar.empty()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		if (m_addressar.size() != 1) throw std::runtime_error(_TXT("only one structure allowed on stack when defining a sentence clause"));

		int addr = m_addressar.back();
		m_addressar.pop_back();

		patchOpCodeJmpAbsolute( addr, m_instructionar.size(), ADDRESS_SUCCESS, m_instructionar.size());
		patchOpCodeJmpAbsolute( addr, m_instructionar.size(), ADDRESS_FAIL, m_instructionar.size()+2);
		pushInstructionFloat( OpWeight, weight);
		pushInstructionString( OpResult, classname);
		if (m_debugtrace)
		{
			m_debugtrace->open( "sentencedef");
			for (int ii=addr; ii<(int)m_instructionar.size(); ++ii)
			{
				std::string instrstr = instructionToString( m_instructionar[ ii]);
				m_debugtrace->event( "instr", "%d %s", ii, instrstr.c_str());
			}
			m_debugtrace->close();
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in define sentence of '%s': %s"), MODULENAME, *m_errorhnd);
}

bool SentenceAnalyzerInstance::compile()
{
	try
	{
		if (!m_addressar.empty()) throw std::runtime_error(_TXT("sentence definitions not complete (stack is not empty)"));
		std::vector<Instruction>::iterator ii = m_instructionar.begin(), ie = m_instructionar.begin();
		for (; ii != ie; ++ii)
		{
			if (isOpCodeJmp( ii->opcode) && ii->arg < 0) throw std::runtime_error(_TXT("not all jump targets resolved"));
		}
		return true;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("errors detected in the automaton definition of the '%s': %s"), MODULENAME, *m_errorhnd, false);
}

std::vector<SentenceGuess> SentenceAnalyzerInstance::analyzeSentence( const SentenceLexerInstanceInterface* lexer, const std::string& sentence) const
{
	try
	{
		std::vector<SentenceGuess> rt;
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in analyze sentence of '%s': %s"), MODULENAME, *m_errorhnd, std::vector<SentenceGuess>());
}



