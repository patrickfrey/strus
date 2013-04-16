#include "strus/operation.hpp"
#include "strus/iterator.hpp"

using namespace strus;

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

	std::vector<Command>::const_iterator ci = m_program->m_cmdlist.begin(), ce = m_program->m_cmdlist.end();
	for (; ci != ce; ++ci)
	{
		switch (ci->m_type)
		{
			case Program::Command::StorageFetch:
				termtype = m_program->m_strings.c_str()+ci->m_operandref[0];
				termvalue = m_program->m_strings.c_str()+ci->m_operandref[1];
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


