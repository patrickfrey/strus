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
#ifndef _STRUS_METADATA_RESTRICTION_HPP_INCLUDED
#define _STRUS_METADATA_RESTRICTION_HPP_INCLUDED
#include "strus/queryInterface.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/private/arithmeticVariantAsString.hpp"
#include <vector>
#include <utility>
#include <iostream>

namespace strus {

struct MetaDataRestriction
{
	typedef bool (*CompareFunction)( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionNotEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionLessFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionLessEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionGreaterFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionGreaterEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionNotEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionLessFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionLessEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionGreaterFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionGreaterEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionNotEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionLessInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionLessEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionGreaterInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionGreaterEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionNotEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionLessUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionLessEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionGreaterUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);
	static bool compareFunctionGreaterEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2);

	static CompareFunction getCompareFunction( const char* type, QueryInterface::CompareOperator cmpop);
	
	MetaDataRestriction( const MetaDataRestriction& o)
		:cmpoperator(o.cmpoperator)
		,func(o.func)
		,elementHandle(o.elementHandle)
		,operand(o.operand)
		,newGroup(o.newGroup){}
	MetaDataRestriction(
			const char* type_,
			QueryInterface::CompareOperator cmpoperator_,
			const Index& elementHandle_,
			const ArithmeticVariant& operand_,
			bool newGroup_)
		:cmpoperator(cmpoperator_)
		,func(getCompareFunction(type_,cmpoperator_))
		,elementHandle(elementHandle_)
		,operand(operand_)
		,newGroup(newGroup_){}

	void print( std::ostream& out) const;
	bool match( MetaDataReaderInterface* md) const;

	QueryInterface::CompareOperator cmpoperator;
	CompareFunction func;
	Index elementHandle;
	ArithmeticVariant operand;
	bool newGroup;
};

bool matchesMetaDataRestriction( const std::vector<MetaDataRestriction>& restr, MetaDataReaderInterface* md);

}
#endif

