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
#ifndef _STRUS_OPERATION_HPP_INCLUDED
#define _STRUS_OPERATION_HPP_INCLUDED
#include "iterator.hpp"
#include "storage.hpp"
#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

namespace strus
{

class Program
{
public:
	struct Match
	{
		DocNumber docnum;
		int ff;
		int dtf;
	};

	struct ResultChunk
	{
		enum {Size=4096};
		int matchsize;
		int matchpos;
		Match match[ Size];
	};

	class Result
	{
	public:
		Result( Storage* storage_, Program* program_);
		PositionIterator* getOperand( int idx) const;

		class iterator
		{
		public:
			const Match& operator*() const;
			iterator& operator++()		{skip(); return *this;}

		private:
			iterator operator++(int)	{throw std::logic_error("postfix increment not implemented");}
			void skip();
			ResultChunk m_chunk;
		};
	private:
		Storage* m_storage;
		Program* m_program;
		typedef boost::shared_ptr<PositionIterator> Operand;
		typedef std::vector<Operand> OperandArray;
		OperandArray m_operandar;
		ResultChunk m_resultChunk;
	};

	struct Command
	{
		enum Type {StorageFetch,Union,IntersectCut,SetRange};
		Type m_type;
		int m_operandref[3];
	};

	void execute( Storage* storage);

private:
	friend class Program::Result;
	std::string m_strings;
	std::vector<Command> m_cmdlist;
};

}//namespace

#endif


