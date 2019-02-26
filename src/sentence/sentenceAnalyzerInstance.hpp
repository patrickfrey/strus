/*
 * Copyright (c) 2019 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the sentence analyzer detecting sentence candidates in queries
#ifndef _STRUS_SENTENCE_ANALYZER_INSTANCE_IMPL_HPP_INCLUDED
#define _STRUS_SENTENCE_ANALYZER_INSTANCE_IMPL_HPP_INCLUDED
#include "strus/sentenceAnalyzerInstanceInterface.hpp"
#include "strus/base/regex.hpp"
#include "strus/reference.hpp"
#include <vector>
#include <string>
#include <iostream>

namespace strus {

/// \brief Forward declaration
class ErrorBufferInterface;
/// \brief Forward declaration
class DebugTraceContextInterface;

/// \brief Implementation of the sentence analyzer detecting sentence candidates in queries
class SentenceAnalyzerInstance
	:public SentenceAnalyzerInstanceInterface
{
public:
	explicit SentenceAnalyzerInstance( ErrorBufferInterface* errorhnd_);

	virtual ~SentenceAnalyzerInstance();

	virtual void pushTerm( const std::string& type, const std::string& name, float weight);

	virtual void pushAlt( int argc);

	virtual void pushSequenceImm( int argc);

	virtual void pushRepeat( int times);

	virtual void defineSentence( const std::string& classname, float weight);

	virtual bool compile();

	virtual std::vector<SentenceGuess> analyzeSentence( const SentenceLexerInstanceInterface* lexer, const std::string& sentence) const;

private:
	enum OpCode {
		OpNop,			///< No operation, goto next instruction
		OpJmp,			///< Absolute jump to instruction address
		OpJmpIf,		///< Jump to absolute address if the compare flag is set
		OpJmpIfNot,		///< Jump to absolute address if the compare flag is not set
		OpStartCount,		///< Push a counter one the counter stack
		OpEndCount,		///< Pop the top counter from the counter stack
		OpDecCount,		///< Decrement the top counter of the counter stack and set the compare flag if the decrementation result got 0
		OpTestType,		///< Set the compare flag if the argument type and the current token are equal, else unset it, combine subsequent OpTest.. operations with a logical AND
		OpTestFeat,		///< Set the compare flag if the argument regular expression matches the current token, else unset it, combine subsequent OpTest.. operations with a logical AND
		OpWeight,		///< Multiply the weight accumulated with this value, the initial weight is 1.0
		OpMark,			///< Push a mark of the current state that is restored with a OpReject on the state stack
		OpReMark,		///< OpRelease+OpMark Pop and forget the top state mark from the state stack and push a mark of the current state that is restored with a OpReject on the state stack
		OpRelease,		///< Pop and forget the top state mark from the state stack
		OpAccept,		///< Push the current token to the result associated with the current state and move the input token cursor forward
		OpReject,		///< Reject the current state and reestablish the state on top of the state stack
		OpResult		///< Add the result associated with the current state to the list of final results
	};
	static const char* opCodeName( OpCode opcode)
	{
		static const char* ar[] = {"NOP","JMP","JIF","JIFNOT","CNT","DEC","TTYPE","TFEAT","WEIGHT","MARK","REMARK","RELEASE","ACCEPT","REJECT","RESULT",0};
		return ar[ opcode];
	}

private:
	struct Instruction
	{
		OpCode opcode;
		int arg;

		Instruction( OpCode opcode_, int arg_)
			:opcode(opcode_),arg(arg_){}
		Instruction( const Instruction& o)
			:opcode(o.opcode),arg(o.arg){}
	};
private:
	void printInstructions( std::ostream& out, int fromAddr, int toAddr) const;
	std::string instructionToString( const Instruction& instr) const;

	void pushInstructionInt( OpCode opcode, int arg);
	void insertInstructionInt( int iaddr, OpCode opcode, int arg);
	void insertInstruction( int iaddr, OpCode opcode);
	void pushInstructionFloat( OpCode opcode, float arg);
	void pushInstructionRegex( OpCode opcode, const std::string& arg);
	void pushInstructionString( OpCode opcode, const std::string& arg);
	void pushInstruction( OpCode opcode);

	static bool isOpCodeJmp( OpCode opcode);
	void patchOpCodeJmpAbsolute( int istart, int iend, int addr, int patchaddr);
	void patchOpCodeJmpOffset( int istart, int addr, int patchaddrincr);

private:
	struct Pattern
	{
		Reference<RegexSearch> regex;
		std::string str;

		Pattern( const std::string& str_, ErrorBufferInterface* errorhnd);
		Pattern( const Pattern& o);
	};

	ErrorBufferInterface* m_errorhnd;
	DebugTraceContextInterface* m_debugtrace;
	std::vector<Instruction> m_instructionar;
	std::vector<Pattern> m_patternar;
	std::vector<std::string> m_valuear;
	std::vector<float> m_weightar;
	std::vector<int> m_addressar;
};

}//namespace
#endif


