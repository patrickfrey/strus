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
#include "strus/sentenceLexerContextInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/debugTraceInterface.hpp"
#include "strus/base/string_format.hpp"
#include "strus/base/local_ptr.hpp"
#include "strus/base/math.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <limits>
#include <iostream>

using namespace strus;

#define MODULENAME "sentence analyzer"
#define STRUS_DBGTRACE_COMPONENT_NAME "sentence"
#undef STRUS_LOWLEVEL_DEBUG

SentenceAnalyzerInstance::SentenceAnalyzerInstance( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_),m_debugtrace(0),m_program()
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


static std::string termListString( const SentenceTermList& terms, const char* sep)
{
	std::string rt;
	SentenceTermList::const_iterator ti = terms.begin(), te = terms.end();
	for (; ti != te; ++ti)
	{
		if (!rt.empty()) rt.append( sep);
		rt.append( strus::string_format( "%s '%s'", ti->type().c_str(), ti->value().c_str()));
	}
	return rt;
}

std::string SentenceAnalyzerInstance::Program::instructionToString( const Instruction& instr) const
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
		case OpJmpDup:
		case OpJmpDupIf:
		case OpJmpDupIfNot:
		case OpStartCount:
			rt.append( strus::string_format( " %d", instr.arg));
			break;
		case OpEndCount:
		case OpDecCount:
			break;
		case OpTestCount:
			rt.append( strus::string_format( " %d", instr.arg));
			break;
		case OpTestType:
			rt.push_back(' ');
			rt.append( valuear[ instr.arg]);
			break;
		case OpTestFeat:
			rt.push_back(' ');
			rt.append( patternar[ instr.arg].str);
			break;
		case OpWeight:
			rt.append( strus::string_format( " %.3f", weightar[ instr.arg]));
			break;
		case OpAccept:
		case OpReject:
			break;
		case OpResult:
			rt.push_back(' ');
			rt.append( valuear[ instr.arg]);
			break;
	}
	return rt;
}

void SentenceAnalyzerInstance::printInstructions( std::ostream& out, int fromAddr, int toAddr) const
{
	if (toAddr < 0) toAddr = m_program.instructionar.size();
	if (fromAddr < 0) fromAddr = 0;
	for (int ip=fromAddr; ip<toAddr; ++ip)
	{
		out << m_program.instructionToString( m_program.instructionar[ ip]) << std::endl;
	}
}

void SentenceAnalyzerInstance::pushInstructionInt( OpCode opcode, int arg)
{
	m_program.instructionar.push_back( Instruction( opcode, arg));
}

void SentenceAnalyzerInstance::pushInstructionFloat( OpCode opcode, float arg)
{
	m_program.instructionar.push_back( Instruction( opcode, m_program.weightar.size()));
	m_program.weightar.push_back( arg);
}

void SentenceAnalyzerInstance::pushInstructionRegex( OpCode opcode, const std::string& arg)
{
	m_program.instructionar.push_back( Instruction( opcode, m_program.patternar.size()));
	m_program.patternar.push_back( Pattern( arg, m_errorhnd));
}

void SentenceAnalyzerInstance::pushInstructionString( OpCode opcode, const std::string& arg)
{
	m_program.instructionar.push_back( Instruction( opcode, m_program.valuear.size()));
	m_program.valuear.push_back( arg);
}

void SentenceAnalyzerInstance::pushInstruction( OpCode opcode)
{
	m_program.instructionar.push_back( Instruction( opcode, 0));
}

void SentenceAnalyzerInstance::insertInstructionInt( int iaddr, OpCode opcode, int arg)
{
	m_program.instructionar.insert( m_program.instructionar.begin() + iaddr, Instruction( opcode, arg));
	patchOpCodeJmpOffset( iaddr, +1);
}

void SentenceAnalyzerInstance::insertInstruction( int iaddr, OpCode opcode)
{
	m_program.instructionar.insert( m_program.instructionar.begin() + iaddr, Instruction( opcode, 0));
	patchOpCodeJmpOffset( iaddr, +1);
}

bool SentenceAnalyzerInstance::isOpCodeJmp( OpCode opcode)
{
	return (opcode == OpJmp || opcode == OpJmpIf || opcode == OpJmpIfNot || opcode == OpJmpDup || opcode == OpJmpDupIf || opcode == OpJmpDupIfNot);
}

void SentenceAnalyzerInstance::patchOpCodeJmpAbsolute( int istart, int iend, int addr, int patchaddr)
{
	std::vector<Instruction>::iterator ii = m_program.instructionar.begin() + istart, ie = m_program.instructionar.begin() + iend;
	for (; ii != ie; ++ii)
	{
		if (isOpCodeJmp( ii->opcode) && ii->arg == addr) ii->arg = patchaddr;
	}
}

void SentenceAnalyzerInstance::patchOpCodeJmpOffset( int addr, int patchaddrincr)
{
	std::vector<Instruction>::iterator ii = m_program.instructionar.begin(), ie = m_program.instructionar.end();
	for (; ii != ie; ++ii)
	{
		if (isOpCodeJmp( ii->opcode) && ii->arg > addr) ii->arg += patchaddrincr;
	}
	std::vector<int>::iterator ai = m_program.addressar.begin(), ae = m_program.addressar.end();
	for (; ai != ae; ++ai)
	{
		if (*ai > addr) *ai += patchaddrincr;
	}
}

void SentenceAnalyzerInstance::eraseInstruction( int iaddr)
{
	int ii = 0, ie = m_program.instructionar.size();
	for (; ii <= iaddr; ++ii)
	{
		Instruction& instr = m_program.instructionar[ ii];
		if (isOpCodeJmp( instr.opcode) && instr.arg > iaddr)
		{
			instr.arg -= 1;
		}
	}
	for (; ii != ie; ++ii)
	{
		Instruction& instr = m_program.instructionar[ ii];
		if (isOpCodeJmp( instr.opcode) && instr.arg > iaddr)
		{
			instr.arg -= 1;
		}
		m_program.instructionar[ ii-1] = m_program.instructionar[ ii];
	}
	m_program.instructionar.resize( m_program.instructionar.size()-1);
	std::vector<int>::iterator ai = m_program.addressar.begin(), ae = m_program.addressar.end();
	for (; ai != ae; ++ai)
	{
		if (*ai >= iaddr) *ai -= 1;
	}
}

#define ADDRESS_SUCCESS -1
#define ADDRESS_TEMPORARY -2

void SentenceAnalyzerInstance::pushTerm( const std::string& type, const std::string& name, float weight)
{
	try
	{
		m_program.addressar.push_back( m_program.instructionar.size());
		if (type == "*" || type.empty())
		{
			if (!name.empty())
			{
				pushInstructionRegex( OpTestFeat, name);
				pushInstructionInt( OpJmpIf, m_program.instructionar.size()+2);
				pushInstruction( OpReject);
			}
		}
		else
		{
			pushInstructionString( OpTestType, type);
			if (!name.empty()) pushInstructionRegex( OpTestFeat, name);
			pushInstructionInt( OpJmpIf, m_program.instructionar.size()+2);
			pushInstruction( OpReject);
		}
		pushInstructionFloat( OpWeight, weight);
		pushInstruction( OpAccept);
		pushInstructionInt( OpJmp, ADDRESS_SUCCESS);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push term: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushNone( float weight)
{
	try
	{
		pushInstructionFloat( OpWeight, weight);
		pushInstructionInt( OpJmp, ADDRESS_SUCCESS);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push none: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushAlt( int argc)
{
	try
	{
		if (argc > (int)m_program.addressar.size()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		if (argc == 0) throw std::runtime_error(_TXT("bad argument, no expressions referenced"));

		int entry_addr = m_program.addressar[ m_program.addressar.size() - argc];
		std::vector<int>::iterator ai = m_program.addressar.begin() + (m_program.addressar.size() - argc), ae = m_program.addressar.end();
		std::vector<int>::iterator ai_next = ai + 1;
		for (; ai_next != ae; ++ai,++ai_next)
		{
			insertInstructionInt( *ai, OpJmpDup, *ai_next);
		}
		m_program.addressar.resize( m_program.addressar.size() - argc + 1);
		m_program.addressar.back() = entry_addr;
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push alt: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushSequenceImm( int argc)
{
	try
	{
		if (argc > (int)m_program.addressar.size()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		if (argc == 0) throw std::runtime_error(_TXT("bad argument, no expressions referenced"));

		std::vector<int>::iterator ai = m_program.addressar.begin() + (m_program.addressar.size() - argc), ae = m_program.addressar.end();
		std::vector<int>::iterator ai_next = ai + 1;
		for (; ai_next != ae; ++ai,++ai_next)
		{
			patchOpCodeJmpAbsolute( *ai, *ai_next, ADDRESS_SUCCESS, *ai_next);
		}
		m_program.addressar.resize( m_program.addressar.size() - argc + 1);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push sequence immediate: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushRepeat( int mintimes, int maxtimes)
{
	try
	{
		if (m_program.addressar.empty()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		if (mintimes < 0) throw std::runtime_error(_TXT("bad argument, min counter has to be a non negative integer"));
		if (mintimes == maxtimes && mintimes == 1) return;

		if (mintimes == 0 && maxtimes == -1)
		{
			// [A] Handle case of '*':
			int loop_addr = m_program.addressar.back();

			insertInstructionInt( loop_addr, OpJmpDup, ADDRESS_SUCCESS);
			patchOpCodeJmpAbsolute( loop_addr+1, m_program.instructionar.size(), ADDRESS_SUCCESS, m_program.instructionar.size());

			pushInstructionInt( OpJmp, loop_addr);
			m_program.addressar.back() = loop_addr;
		}
		else
		{
			// [B] Handle case of '{start,end}':
			if (mintimes > maxtimes) throw std::runtime_error(_TXT("bad argument, minimes has to be bigger or equal to max times"));
			if (maxtimes <= 0) throw std::runtime_error(_TXT("bad argument, max counter has to be a positive integer or -1 (unlimited)"));

			int entry_addr = m_program.addressar.back();
	
			insertInstructionInt( entry_addr, OpStartCount, maxtimes);
			int loop_addr = entry_addr+1;
			int ins_addr = loop_addr;

			if (mintimes < maxtimes)
			{
				// ... create an instance done if we reached the minimum count
				insertInstructionInt( ins_addr, OpTestCount, maxtimes-mintimes+1);
				ins_addr++;
				insertInstructionInt( ins_addr, OpJmpDupIfNot, ADDRESS_TEMPORARY);
				ins_addr++;
			}
			patchOpCodeJmpAbsolute( ins_addr, m_program.instructionar.size(), ADDRESS_SUCCESS, m_program.instructionar.size());
	
			pushInstruction( OpDecCount);
			pushInstructionInt( OpJmpIfNot, loop_addr);
			patchOpCodeJmpAbsolute( loop_addr, ins_addr, ADDRESS_TEMPORARY, m_program.instructionar.size());
			pushInstruction( OpEndCount);
			pushInstructionInt( OpJmp, ADDRESS_SUCCESS);
			m_program.addressar.back() = entry_addr;
		}
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push repeat: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::defineSentence( const std::string& classname, float weight)
{
	try
	{
		if (m_program.addressar.empty()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		if (m_program.addressar.size() != 1) throw std::runtime_error(_TXT("only one structure allowed on stack when defining a sentence clause"));

		int addr = m_program.addressar.back();
		m_program.procar.push_back( addr);
		m_program.addressar.pop_back();

		patchOpCodeJmpAbsolute( addr, m_program.instructionar.size(), ADDRESS_SUCCESS, m_program.instructionar.size());
		pushInstructionFloat( OpWeight, weight);
		pushInstructionString( OpResult, classname);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in define sentence of '%s': %s"), MODULENAME, *m_errorhnd);
}

bool SentenceAnalyzerInstance::compile()
{
	try
	{
		if (!m_program.addressar.empty()) throw std::runtime_error(_TXT("sentence definitions not complete (stack is not empty)"));
		int ii = 0, ie = m_program.instructionar.size();
		for (ii=0; ii != ie; ++ii)
		{
			Instruction& instr = m_program.instructionar[ ii];
			if (instr.opcode == OpJmp)
			{
				if (instr.arg == ii+1)
				{
					// Optimization: Elimination of jump to the next address
					eraseInstruction( ii);
					--ie;
				}
			}
		}
		for (ii=0; ii != ie; ++ii)
		{
			Instruction& instr = m_program.instructionar[ ii];
			if (isOpCodeJmp( instr.opcode))
			{
				if (instr.arg < 0) throw std::runtime_error(_TXT("unresolved jump target"));
				if (instr.arg > ie) throw std::runtime_error(_TXT("invalid jump target"));
			}
		}
		if (m_debugtrace)
		{
			m_debugtrace->open( "program");
			std::vector<int>::const_iterator pi = m_program.procar.begin(), pe = m_program.procar.end();
			for (ii=0; ii<ie; ++ii)
			{
				const char* prefix = "";
				if (pi < pe && ii == *pi)
				{
					prefix = "*";
					++pi;
				}
				std::string instrstr = m_program.instructionToString( m_program.instructionar[ ii]);
				m_debugtrace->event( "instr", "%d%s %s", ii, prefix, instrstr.c_str());
			}
			m_debugtrace->close();

			std::string procarstr;
			pi = m_program.procar.begin();
			for (; pi != pe; ++pi)
			{
				if (!procarstr.empty()) procarstr.append(", ");
				procarstr.append( strus::string_format( "%d", *pi));
			}
			m_debugtrace->event( "procs", "addr %s", procarstr.c_str());
		}
		return true;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("errors detected in the automaton definition of the '%s': %s"), MODULENAME, *m_errorhnd, false);
}

struct ExecutionData
{
	ExecutionData() :elementLists(),counterLists(){}

	struct Element
	{
		int termidx;
		int prev;

		explicit Element( int termidx_, int prev_)
			:termidx(termidx_),prev(prev_){}
		Element( const Element& o)
			:termidx(o.termidx),prev(o.prev){}
	};
	struct Counter
	{
		int value;
		int prev;

		Counter( int value_, int prev_)
			:value(value_),prev(prev_){}
		Counter( const Counter& o)
			:value(o.value),prev(o.prev){}
	};

	int pushElement( int list, int termidx)
	{
		elementLists.push_back( Element( termidx, list));
		return elementLists.size()-1;
	}

	SentenceTermList elementList( int list, const SentenceTermList& termlist) const
	{
		SentenceTermList rt;
		for (; list >= 0; list = elementLists[ list].prev)
		{
			rt.push_back( termlist[ elementLists[ list].termidx]);
		}
		std::reverse( rt.begin(), rt.end());
		return rt;
	}

	int copyCounters( int list)
	{
		if (list == -1) return list;
		counterLists.push_back( Counter( counterLists[ list].value, copyCounters( counterLists[ list].prev)));
		return counterLists.size()-1;
	}

	int pushCounter( int list, int value)
	{
		counterLists.push_back( Counter( value, list));
		return counterLists.size()-1;
	}

	int popCounter( int list)
	{
		if (list == -1) throw std::runtime_error(_TXT("logic error: illegal pop from empty counter stack"));
		return counterLists[ list].prev;
	}

	bool decCounter( int list)
	{
		if (list == -1) throw std::runtime_error(_TXT("logic error: illegal access of element of empty counter stack"));
		return 0==--counterLists[ list].value;
	}
	bool testCounter( int list, int minval)
	{
		if (list == -1) throw std::runtime_error(_TXT("logic error: illegal access of element of empty counter stack"));
		return minval <= counterLists[ list].value;
	}

	std::vector<Element> elementLists;
	std::vector<Counter> counterLists;
};

struct ExecutionState
{
	bool testflag;
	int ip;
	int counters;
	int elements;
	double weight;

	explicit ExecutionState( int entryaddr)
		:testflag(true),ip(entryaddr),counters(-1),elements(-1),weight(1.0)
	{}
	ExecutionState( const ExecutionState& o)
		:testflag(o.testflag),ip(o.ip),counters(o.counters),elements(o.elements),weight(o.weight)
	{}
	ExecutionState& operator = ( const ExecutionState& o)
	{
		testflag = o.testflag;
		ip = o.ip;
		counters = o.counters;
		elements = o.elements;
		weight = o.weight;
		return *this;
	}
	ExecutionState duplicate( ExecutionData& execdata, int ip_) const
	{
		ExecutionState rt( *this);
		rt.ip = ip_;
		rt.counters = execdata.copyCounters( counters);
		return rt;
	}
	ExecutionState duplicate( ExecutionData& execdata) const
	{
		ExecutionState rt( *this);
		rt.counters = execdata.copyCounters( counters);
		return rt;
	}
	std::string tostring( const ExecutionData& execdata) const
	{
		int elementcnt = 0;
		int ei = elements;
		while (ei >= 0)
		{
			ei = execdata.elementLists[ ei].prev;
			++elementcnt;
		}
		std::string counterstr;
		int ci = counters;
		while (ci >= 0)
		{
			if (!counterstr.empty()) counterstr.append(",");
			counterstr.append( strus::string_format("%d", execdata.counterLists[ ci].value));
			ci = execdata.counterLists[ ci].prev;
		}
		return strus::string_format("flag: %s, ip: %d, counters:(%s), elements:%d, weight:%.3f", (testflag?"true":"false"), ip, counterstr.c_str(), elementcnt, weight);
	}
	int compare(const ExecutionState& o) const
	{
		if (strus::Math::abs( weight - o.weight) <= std::numeric_limits<float>::epsilon())
		{
			if (ip == o.ip)
			{
				if (counters == o.counters)
				{
					return elements - o.elements;
				}
				else return counters - o.counters;
			}
			else return ip - o.ip;
		}
		else return weight < o.weight ? -1 : +1;
	}
	bool operator < (const ExecutionState& o) const		{return (compare( o) < 0);}
	bool operator <= (const ExecutionState& o) const	{return (compare( o) <= 0);}
	bool operator == (const ExecutionState& o) const	{return (compare( o) == 0);}

	enum FeedReturn {FeedResult,FeedAccept,FeedReject};
	FeedReturn feed( const SentenceAnalyzerInstance::Program& program, ExecutionData& data, std::vector<ExecutionState>& states, int termidx, const SentenceTerm& term, DebugTraceContextInterface* debugtrace)
	{
		while (ip < (int)program.instructionar.size())
		{
			const SentenceAnalyzerInstance::Instruction& instr = program.instructionar[ ip++];
			if (debugtrace)
			{
				std::string instrstr = program.instructionToString( instr);
				debugtrace->event( "do", "%d %s [%s]", ip-1, instrstr.c_str(), testflag?"T":"F");
			}
			switch (instr.opcode)
			{
				case SentenceAnalyzerInstance::OpNop:
					testflag = true;
					break;
				case SentenceAnalyzerInstance::OpJmp:
					ip = instr.arg;
					testflag = true;
					break;
				case SentenceAnalyzerInstance::OpJmpIf:
					if (testflag) ip = instr.arg;
					testflag = true;
					break;
				case SentenceAnalyzerInstance::OpJmpIfNot:
					if (!testflag) ip = instr.arg;
					testflag = true;
					break;
				case SentenceAnalyzerInstance::OpJmpDup:
					testflag = true;
					states.push_back( duplicate( data, instr.arg));
					break;
				case SentenceAnalyzerInstance::OpJmpDupIf:
					if (testflag)
					{
						states.push_back( duplicate( data, instr.arg));
					}
					else
					{
						testflag = true;
					}
					break;
				case SentenceAnalyzerInstance::OpJmpDupIfNot:
					if (!testflag)
					{
						testflag = true;
						states.push_back( duplicate( data, instr.arg));
					}
					break;
				case SentenceAnalyzerInstance::OpStartCount:
					counters = data.pushCounter( counters, instr.arg);
					testflag = true;
					break;
				case SentenceAnalyzerInstance::OpEndCount:
					counters = data.popCounter( counters);
					testflag = true;
					break;
				case SentenceAnalyzerInstance::OpDecCount:
					testflag = data.decCounter( counters);
					break;
				case SentenceAnalyzerInstance::OpTestCount:
					testflag &= data.testCounter( counters, instr.arg);
					break;
				case SentenceAnalyzerInstance::OpTestType:
					if (termidx < 0) return FeedReject;
					testflag &= term.type() == program.valuear[ instr.arg];
					break;
				case SentenceAnalyzerInstance::OpTestFeat:
					if (termidx < 0) return FeedReject;
					testflag &= program.patternar[ instr.arg].regex->match( term.value());
					break;
				case SentenceAnalyzerInstance::OpWeight:
					weight *= program.weightar[ instr.arg];
					testflag = true;
					break;
				case SentenceAnalyzerInstance::OpAccept:
					if (termidx < 0) return FeedReject;
					elements = data.pushElement( elements, termidx);
					return FeedAccept;
				case SentenceAnalyzerInstance::OpReject:
					return FeedReject;
				case SentenceAnalyzerInstance::OpResult:
					return FeedResult;
			}
		}
		return FeedReject;
	}

	SentenceGuess result( const SentenceAnalyzerInstance::Program& program, ExecutionData& data, std::vector<ExecutionState>& states, const SentenceTermList& termlist, DebugTraceContextInterface* debugtrace)
	{
		SentenceTerm term;
		if (debugtrace) debugtrace->open( "instr");

		if (FeedResult == feed( program, data, states, -1, term, debugtrace))
		{
			if (ip == 0 || program.instructionar[ ip-1].opcode != SentenceAnalyzerInstance::OpResult)
			{
				throw std::runtime_error(_TXT("logic error: expected result instruction"));
			}
			if (debugtrace) debugtrace->close();
			return SentenceGuess( program.valuear[ program.instructionar[ ip-1].arg], data.elementList( elements, termlist), weight);
		}
		else
		{
			if (debugtrace) debugtrace->close();
			return SentenceGuess();
		}
	}
};

struct SentenceAnalyzerInstance::ExecutionContext
{
	typedef std::map<SentenceTerm,int> TermMap;

	TermMap termmap;
	std::vector<SentenceTerm> termar;
	std::vector<SentenceGuess> results;
	double maxWeight;

	ExecutionContext()
		:termmap(),termar(),results(),maxWeight(0.0){}
};

void SentenceAnalyzerInstance::SentenceAnalyzerInstance::analyzeTermList( ExecutionContext& exectx, const SentenceLexerContextInterface* lexerctx) const
{
	std::vector<ExecutionState> init_states;
	std::vector<ExecutionState> accept_states;

	std::vector<int>::const_iterator pi = m_program.procar.begin(), pe = m_program.procar.end();
	for (; pi != pe; ++pi)
	{
		init_states.push_back( ExecutionState( *pi));
	}
	ExecutionData execdata;
	if (m_debugtrace) m_debugtrace->open( "instr");

	int ki = 0, ke = lexerctx->nofTokens();
	for (; ki != ke; ++ki)
	{
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "TOKEN " << strus::string_format("%d of %d", ki, ke) << std::endl;
#endif
		std::string feat = lexerctx->featureValue( ki);
		std::vector<std::string> types = lexerctx->featureTypes( ki);

		std::vector<std::string>::const_iterator ti = types.begin(), te = types.end();
		for (; ti != te; ++ti)
		{
			SentenceTerm term( *ti, feat);
#ifdef STRUS_LOWLEVEL_DEBUG
			std::cerr << "TERM " << strus::string_format("type %s value '%s'", ti->c_str(), feat.c_str()) << std::endl;
#endif
			ExecutionContext::TermMap::const_iterator mi = exectx.termmap.find( term);
			int termidx = mi == exectx.termmap.end() ? exectx.termar.size() : mi->second;

			std::vector<ExecutionState> states = init_states;
			std::size_t sidx = 0;
			for (; sidx < states.size(); ++sidx)
			{
				ExecutionState state = states[ sidx].duplicate( execdata);
#ifdef STRUS_LOWLEVEL_DEBUG
				std::cerr << "STATE " << state.tostring( execdata) << std::endl;
#endif
				switch (state.feed( m_program, execdata, states, termidx, term, m_debugtrace))
				{
					case ExecutionState::FeedResult:
#ifdef STRUS_LOWLEVEL_DEBUG
						std::cerr << "RESULT " << state.tostring( execdata) << std::endl;
#endif
						break;
					case ExecutionState::FeedAccept:
						if (state.weight - std::numeric_limits<float>::epsilon() > exectx.maxWeight)
						{
							accept_states.clear();
						}
						else if (state.weight + std::numeric_limits<float>::epsilon() < exectx.maxWeight)
						{
							break;
						}
						exectx.maxWeight = state.weight;
						if (termidx == (int)exectx.termar.size())
						{
							exectx.termmap[ term] = termidx;
							exectx.termar.push_back( term);
						}
#ifdef STRUS_LOWLEVEL_DEBUG
						std::cerr << "ACCEPT " << state.tostring( execdata) << std::endl;
#endif
						accept_states.push_back( state);
						break;
					case ExecutionState::FeedReject:
#ifdef STRUS_LOWLEVEL_DEBUG
						std::cerr << "REJECT " << state.tostring( execdata) << std::endl;
#endif
						break;
				}
			}
		}
		init_states.clear();
		init_states.swap( accept_states);
	}
	std::size_t sidx = 0;
	for (; sidx != init_states.size(); ++sidx)
	{
		ExecutionState state = init_states[ sidx];
		if (state.weight + std::numeric_limits<float>::epsilon() > exectx.maxWeight)
		{
			if (state.weight - std::numeric_limits<float>::epsilon() > exectx.maxWeight)
			{
				exectx.results.clear();
			}
			else if (state.weight + std::numeric_limits<float>::epsilon() < exectx.maxWeight)
			{
				continue;
			}
			SentenceGuess result = state.result( m_program, execdata, init_states, exectx.termar, m_debugtrace);
			if (result.valid())
			{
				exectx.results.push_back( result);
			}
		}
	}
	if (m_debugtrace) m_debugtrace->close();
}


std::vector<SentenceGuess> SentenceAnalyzerInstance::analyzeSentence( const SentenceLexerInstanceInterface* lexer, const std::string& source, int maxNofResults, double minWeight) const
{
	try
	{
		strus::local_ptr<SentenceLexerContextInterface> lexerctx( lexer->createContext( source));
		if (!lexerctx.get()) throw std::runtime_error(_TXT("failed to create lexer context"));

		ExecutionContext exectx;

		bool more = lexerctx->fetchFirstSplit();
		for (; more; more = lexerctx->fetchNextSplit())
		{
			analyzeTermList( exectx, lexerctx.get());
		}
		std::vector<SentenceGuess> rt = lexerctx->rankSentences( exectx.results, maxNofResults);
		std::vector<SentenceGuess>::const_iterator ri = rt.begin(), re = rt.end();
		int ridx = 0;
		for (; ri != re && ri->weight() + std::numeric_limits<float>::epsilon() >= minWeight; ++ri,++ridx){}
		rt.resize( ridx);
		if (m_errorhnd->hasError())
		{
			throw std::runtime_error( m_errorhnd->fetchError());
		}
		if (m_debugtrace)
		{
			m_debugtrace->open( "result");
			ri = rt.begin();
			for (; ri != re; ++ri)
			{
				std::string termliststr = termListString( ri->terms(), ", ");
				m_debugtrace->event( "guess", "%.3f '%s' (%s)", ri->weight(), ri->classname().c_str(), termliststr.c_str());
			}
			m_debugtrace->close();
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in analyze sentence of '%s': %s"), MODULENAME, *m_errorhnd, std::vector<SentenceGuess>());
}



