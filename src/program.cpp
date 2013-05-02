/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2013 Patrick Frey

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
#include "strus/program.hpp"
#include "strus/iterator.hpp"

using namespace strus;

Program::Program::Command( const Command& o)
	:m_type(o.m_type),m_operandref(o.m_operandref){}

Program::Program::Command( Type type_, int op1, int op2, int op3)
	:m_type(type_)
{
	m_operandref[0] = op1;
	m_operandref[1] = op2;
	m_operandref[2] = op3;
}

Program::Program()
	:m_nofrefarg(0)
{}

Program::Program( const Program& o)
	:m_strings(o.m_strings)
	,m_cmdlist(o.m_cmdlist)
	,m_refargset(o.m_refargset)
	,m_nofrefarg(o.m_nofrefarg)
{}

void Program::FETCH( const std::string& type, const std::string& value)
{
	m_cmdlist.push_back( Command( StorageFetch, m_strings.size(), m_strings.size()+1));
	m_strings.push_back( type);
	m_strings.push_back( value);
}

void Program::refarg( int arg) const
{
	while (m_refargset.size() < m_cmdlist.size())
	{
		m_refargset.push_back( false);
	}
	std::size_t idx = (arg<0)?(m_cmdlist.size()+arg):(std::size_t)arg;
	if (arg != 0)
	{
		if (m_refargset.size() >= idx)
		{
			throw std::runtime_error( "illegal address referenced in program");
		}
		if (m_refargset.at( idx))
		{
			throw std::runtime_error( "address in program referenced twice");
		}
		else
		{
			m_refargset[ idx] = true;
			++m_nofrefarg;
		}
	}
}

void Program::UNION( int select1_, int select2_)
{
	m_cmdlist.push_back( Command( Union, select1_, select2_));
	refarg( select1_);
	refarg( select2_);
}

void Program::INTERSECT( int select_, int joincond_)
{
	m_cmdlist.push_back( Command( IntersectCut, select_, joincond_));
	refarg( select_);
	refarg( joincond_);
}

void Program::JOINCUT( int select_, int joincond_, int cutcond)
{
	m_cmdlist.push_back( Command( IntersectCut, select_, joincond_, cutcond_));
	refarg( select_);
	refarg( joincond_);
	refarg( cutcond_);
}

void Program::RANGE( int rangestart_, int range_)
{
	m_cmdlist.push_back( Command( SetRange, rangestart_, range_));
}

const char* Program::check() const
{
	if (m_nofrefarg < m_refargset.size()-1)
	{
		return "program is incomplete. not all elements declared are referenced and contributing to the result";
	}
	return 0;
}


PositionIterator* Program::Result::getOperand( int idx) const
{
	std::size_t aridx = 0;
	if (idx == 0)
	{
		return 0;
	}
	else if (idx < 0)
	{
		aridx = (std::size_t)(m_operandar.size()+idx);
	}
	else
	{
		aridx = (std::size_t)idx;
	}
	if (aridx >= m_operandar.size()) throw std::runtime_error( "operand index out of range in program");
	return m_operandar.at( aridx).get();
}

Program::Result::Result( Storage* storage_, Program* program_)
	:m_storage(storage_)
	,m_program(program_)
{
	TermNumber termnumber;
	const char* termtype;
	const char* termvalue;
	int rangestart = 0;
	int range = -1;

	const char* msg = m_program->check();
	if (msg)
	{
		throw std::runtime_error( std::string("program check failed:") + msg);
	}
	if (m_program->cmdlist.empty())
	{
		m_operandar.push_back( Operand( new PositionIterator));
		return;
	}
	std::vector<Command>::const_iterator ci = m_program->m_cmdlist.begin(), ce = m_program->m_cmdlist.end();
	for (; ci != ce; ++ci)
	{
		switch (ci->m_type)
		{
			case Program::Command::StorageFetch:
				termtype = m_program->m_strings[ ci->m_operandref[ 0]];
				termvalue = m_program->m_strings[ ci->m_operandref[ 1]];
				termnumber = m_storage->getTermNumber( termtype, termvalue);
				m_operandar.push_back(
					Operand( new StoragePositionIterator( m_storage, termnumber))
				);
				break;
			break;
			case Program::Command::Union:
				m_operandar.push_back(
					Operand( new UnionPositionIterator(
						getOperand( ci->m_operandref[0]),
						getOperand( ci->m_operandref[1])))
				);
			break;
			case Program::Command::IntersectCut:
				m_operandar.push_back(
					Operand( new IntersectionCutPositionIterator(
						getOperand( ci->m_operandref[0]),
						getOperand( ci->m_operandref[1]),
						rangestart, range,
						getOperand( ci->m_operandref[1])))
				);
			break;
			case Program::Command::SetRange:
				rangestart = ci->m_operandref[0];
				range = ci->m_operandref[1];
			break;
		}
	}
}

bool Program::Result::getNext( Match& match)
{
	match.ff = 0;
	Position pos = m_operandar.back()->get();
	if (!pos)
	{
		if (!m_operandar.back()->eof())
		{
			m_operandar.back()->fetch();
		}
		if (!pos) return false;
	}
	match.docnum = Encode::getDocNumber( pos);
	do
	{
		m_operandar.back()->next();
		pos = m_operandar.back()->get();
		match.ff += 1;
	}
	while (match.docnum == Encode::getDocNumber( pos));
	return true;
}


