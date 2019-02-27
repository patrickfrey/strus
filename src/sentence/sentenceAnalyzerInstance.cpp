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
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <vector>
#include <map>
#include <string>
#include <algorithm>

using namespace strus;

#define MODULENAME "sentence analyzer"
#define STRUS_DBGTRACE_COMPONENT_NAME "sentence"

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
		case OpJmpDup:
		case OpStartCount:
			rt.append( strus::string_format( " %d", instr.arg));
			break;
		case OpEndCount:
		case OpDecCount:
			break;
		case OpTestType:
			rt.push_back(' ');
			rt.append( m_program.valuear[ instr.arg]);
			break;
		case OpTestFeat:
			rt.push_back(' ');
			rt.append( m_program.patternar[ instr.arg].str);
			break;
		case OpWeight:
			rt.append( strus::string_format( " %.3f", m_program.weightar[ instr.arg]));
			break;
		case OpAccept:
		case OpReject:
			break;
		case OpResult:
			rt.push_back(' ');
			rt.append( m_program.valuear[ instr.arg]);
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
		out << instructionToString( m_program.instructionar[ ip]) << std::endl;
	}
}

void SentenceAnalyzerInstance::pushInstructionInt( OpCode opcode, int arg)
{
	m_program.instructionar.push_back( Instruction( opcode, arg));
}

void SentenceAnalyzerInstance::pushInstructionFloat( OpCode opcode, float arg)
{
	m_program.weightar.push_back( arg);
	m_program.instructionar.push_back( Instruction( opcode, m_program.weightar.size()));
}

void SentenceAnalyzerInstance::pushInstructionRegex( OpCode opcode, const std::string& arg)
{
	m_program.patternar.push_back( Pattern( arg, m_errorhnd));
	m_program.instructionar.push_back( Instruction( opcode, m_program.patternar.size()));
}

void SentenceAnalyzerInstance::pushInstructionString( OpCode opcode, const std::string& arg)
{
	m_program.valuear.push_back( arg);
	m_program.instructionar.push_back( Instruction( opcode, m_program.valuear.size()));
}

void SentenceAnalyzerInstance::pushInstruction( OpCode opcode)
{
	m_program.instructionar.push_back( Instruction( opcode, 0));
}

void SentenceAnalyzerInstance::insertInstructionInt( int iaddr, OpCode opcode, int arg)
{
	patchOpCodeJmpOffset( iaddr, iaddr, +1);
	m_program.instructionar.insert( m_program.instructionar.begin() + iaddr, Instruction( opcode, arg));
}

void SentenceAnalyzerInstance::insertInstruction( int iaddr, OpCode opcode)
{
	patchOpCodeJmpOffset( iaddr, iaddr, +1);
	m_program.instructionar.insert( m_program.instructionar.begin() + iaddr, Instruction( opcode, 0));
}

bool SentenceAnalyzerInstance::isOpCodeJmp( OpCode opcode)
{
	return (opcode == OpJmp || opcode == OpJmpIf || opcode == OpJmpIfNot || opcode == OpJmpDup);
}

void SentenceAnalyzerInstance::patchOpCodeJmpAbsolute( int istart, int iend, int addr, int patchaddr)
{
	std::vector<Instruction>::iterator ii = m_program.instructionar.begin() + istart, ie = m_program.instructionar.begin() + iend;
	for (; ii != ie; ++ii)
	{
		if (isOpCodeJmp( ii->opcode) && ii->arg == addr) ii->arg = patchaddr;
	}
}

void SentenceAnalyzerInstance::patchOpCodeJmpOffset( int istart, int addr, int patchaddrincr)
{
	std::vector<Instruction>::iterator ii = m_program.instructionar.begin() + istart, ie = m_program.instructionar.end();
	for (; ii != ie; ++ii)
	{
		if (isOpCodeJmp( ii->opcode) && ii->arg >= addr) ii->arg += patchaddrincr;
	}
	std::vector<int>::iterator ai = m_program.addressar.begin(), ae = m_program.addressar.end();
	for (; ai != ae; ++ai)
	{
		if (*ai >= istart) *ai += patchaddrincr;
	}
}


#define ADDRESS_SUCCESS -1

void SentenceAnalyzerInstance::pushTerm( const std::string& type, const std::string& name, float weight)
{
	try
	{
		m_program.addressar.push_back( m_program.instructionar.size());
		pushInstructionString( OpTestType, type);
		if (!name.empty()) pushInstructionRegex( OpTestFeat, name);
		pushInstructionInt( OpJmpIf, m_program.instructionar.size()+1);
		pushInstruction( OpReject);
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
		if (argc > (int)m_program.addressar.size()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		if (argc == 0) throw std::runtime_error(_TXT("bad argument, no expressions referenced"));

		std::vector<int>::iterator ai = m_program.addressar.begin() + (m_program.addressar.size() - argc), ae = m_program.addressar.end();
		std::vector<int>::iterator ai_next = ai + 1;
		for (; ai_next != ae; ++ai,++ai_next)
		{
			insertInstructionInt( *ai, OpJmpDup, *ai_next);
		}
		m_program.addressar.resize( m_program.addressar.size() - argc + 1);
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
		std::vector<int>::iterator ai_next = m_program.addressar.begin() + 1;
		for (; ai_next != ae; ++ai,++ai_next)
		{
			patchOpCodeJmpAbsolute( *ai, *ai_next, ADDRESS_SUCCESS, *ai_next);
		}
		m_program.addressar.resize( m_program.addressar.size() - argc + 1);
	}
	CATCH_ERROR_ARG1_MAP( _TXT("error in '%s' operation push sequence immediate: %s"), MODULENAME, *m_errorhnd);
}

void SentenceAnalyzerInstance::pushRepeat( int times)
{
	try
	{
		if (m_program.addressar.empty()) throw std::runtime_error(_TXT("bad argument, not enough expressions on the stack"));
		if (times <= 0) throw std::runtime_error(_TXT("bad argument, counter has to be a positive integer"));
		int addr = m_program.addressar.back();

		insertInstructionInt( addr++, OpStartCount, times);
		pushInstructionInt( OpJmpDup, m_program.instructionar.size()+2);

		patchOpCodeJmpAbsolute( addr, m_program.instructionar.size(), ADDRESS_SUCCESS, m_program.instructionar.size());

		pushInstruction( OpDecCount);
		pushInstructionInt( OpJmpIf, addr/*address of OpJmpDup*/);
		pushInstruction( OpEndCount);
		pushInstructionInt( OpJmp, ADDRESS_SUCCESS);
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
		if (m_debugtrace)
		{
			m_debugtrace->open( "sentencedef");
			for (int ii=addr; ii<(int)m_program.instructionar.size(); ++ii)
			{
				std::string instrstr = instructionToString( m_program.instructionar[ ii]);
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
		if (!m_program.addressar.empty()) throw std::runtime_error(_TXT("sentence definitions not complete (stack is not empty)"));
		std::vector<Instruction>::iterator ii = m_program.instructionar.begin(), ie = m_program.instructionar.begin();
		for (; ii != ie; ++ii)
		{
			if (isOpCodeJmp( ii->opcode) && ii->arg < 0) throw std::runtime_error(_TXT("not all jump targets resolved"));
		}
		if (m_debugtrace)
		{
			std::string procarstr;
			std::vector<int>::const_iterator pi = m_program.procar.begin(), pe = m_program.procar.end();
			for (; pi != pe; ++pi)
			{
				if (!procarstr.empty()) procarstr.append(", ");
				procarstr.append( strus::string_format( "%d", *pi));
			}
			m_debugtrace->event( "procs", "%s", procarstr.c_str());
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
	ExecutionState& withIp( int ip_)
	{
		ip = ip_;
		return *this;
	}

	int compare(const ExecutionState& o) const
	{
		if (std::abs( weight - o.weight) <= std::numeric_limits<float>::epsilon())
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
	FeedReturn feed( const SentenceAnalyzerInstance::Program& program, ExecutionData& data, std::vector<ExecutionState>& states, int termidx, const SentenceTerm& term)
	{
		while (ip < (int)program.instructionar.size())
		{
			const SentenceAnalyzerInstance::Instruction& instr = program.instructionar[ ip++];
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
					states.push_back( ExecutionState( *this).withIp( instr.arg));
					testflag = true;
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
				case SentenceAnalyzerInstance::OpTestType:
					if (termidx < 0) return FeedReject;
					testflag &= term.type() == program.valuear[ instr.arg];
					break;
				case SentenceAnalyzerInstance::OpTestFeat:
					if (termidx < 0) return FeedReject;
					testflag &= program.patternar[ instr.arg].regex->match_complete( term.value());
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

	SentenceGuess result( const SentenceAnalyzerInstance::Program& program, ExecutionData& data, const SentenceTermList& termlist)
	{
		std::vector<ExecutionState> states;
		SentenceTerm term;
		if (FeedResult != feed( program, data, states, -1, term)) return SentenceGuess();
		if (ip == 0 || program.instructionar[ ip-1].opcode != SentenceAnalyzerInstance::OpResult) return SentenceGuess();

		return SentenceGuess( program.valuear[ program.instructionar[ ip-1].arg], data.elementList( elements, termlist), weight);
	}
};



std::vector<SentenceGuess> SentenceAnalyzerInstance::analyzeSentence( const SentenceLexerInstanceInterface* lexer, const std::string& source, int maxNofResults) const
{
	try
	{
		strus::local_ptr<SentenceLexerContextInterface> lexerctx( lexer->createContext( source));
		std::vector<SentenceGuess> results;
		ExecutionData data;
		typedef std::map<SentenceTerm,int> TermMap;
		TermMap termmap;
		std::vector<SentenceTerm> termar;
		std::vector<ExecutionState> init_states;

		std::vector<int>::const_iterator pi = m_program.procar.begin(), pe = m_program.procar.begin();
		for (; pi != pe; ++pi)
		{
			init_states.push_back( ExecutionState( *pi));
		}
		double maxWeight = 0.0;

		bool more = lexerctx->fetchFirstSplit();
		for (; more; more = lexerctx->fetchNextSplit())
		{
			std::vector<ExecutionState> start_states = init_states;
			std::vector<ExecutionState> accept_states;

			int ki = 0, ke = lexerctx->nofTokens();
			for (; ki != ke; ++ki)
			{
				std::string feat = lexerctx->featureValue( ki);
				std::vector<std::string> types = lexerctx->featureTypes( ki);

				std::vector<std::string>::const_iterator ti = types.begin(), te = types.end();
				for (; ti != te; ++ti)
				{
					SentenceTerm term( *ti, feat);
					TermMap::const_iterator mi = termmap.find( term);
					int termidx = mi == termmap.end() ? termar.size() : mi->second;

					std::vector<ExecutionState> states = start_states;
					while (!states.empty())
					{
						std::vector<ExecutionState> dup_states;
						std::vector<ExecutionState>::iterator si = states.begin(), se = states.end();
						for (; si != se; ++si)
						{
							switch (si->feed( m_program, data, dup_states, termidx, term))
							{
								case ExecutionState::FeedResult:
									break;
								case ExecutionState::FeedAccept:
									if (si->weight - std::numeric_limits<float>::epsilon() > maxWeight)
									{
										accept_states.clear();
									}
									else if (si->weight + std::numeric_limits<float>::epsilon() < maxWeight)
									{
										break;
									}
									maxWeight = si->weight;
									if (termidx == (int)termar.size())
									{
										termmap[ term] = termidx;
										termar.push_back( term);
									}
									accept_states.push_back( *si);
									break;
								case ExecutionState::FeedReject:
									break;
							}
						}
						states = dup_states;
					}
				}
				start_states = accept_states;
			}
			std::vector<ExecutionState>::iterator ai = accept_states.begin(), ae = accept_states.end();
			for (; ai != ae; ++ai)
			{
				if (ai->weight + std::numeric_limits<float>::epsilon() > maxWeight)
				{
					if (ai->weight - std::numeric_limits<float>::epsilon() > maxWeight)
					{
						results.clear();
					}
					else if (ai->weight + std::numeric_limits<float>::epsilon() < maxWeight)
					{
						continue;
					}
					SentenceGuess result = ai->result( m_program, data, termar);
					if (result.valid())
					{
						results.push_back( result);
					}
				}
			}
		}
		std::vector<SentenceGuess> rt = lexerctx->rankSentences( results, maxNofResults);
		if (m_errorhnd->hasError())
		{
			throw std::runtime_error( m_errorhnd->fetchError());
		}
		if (m_debugtrace)
		{
			m_debugtrace->open( "result");
			std::vector<SentenceGuess>::const_iterator ri = rt.begin(), re = rt.end();
			for (; ri != re; ++ri)
			{
				std::string termliststr = termListString( ri->terms(), ", ");
				m_debugtrace->event( "guess", "%.3f %s %s", ri->weight(), ri->classname().c_str(), termliststr.c_str());
			}
			m_debugtrace->close();
		}
		return rt;
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in analyze sentence of '%s': %s"), MODULENAME, *m_errorhnd, std::vector<SentenceGuess>());
}



