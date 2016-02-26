/*
---------------------------------------------------------------------
    The C++ library strus implements basic operations to build
    a search engine for structured search on unstructured data.

    Copyright (C) 2015 Patrick Frey

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 3 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

--------------------------------------------------------------------

	The latest version of strus can be found at 'http://github.com/patrickfrey/strus'
	For documentation see 'http://patrickfrey.github.com/strus'

--------------------------------------------------------------------
*/
#include "metaDataRestriction.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "private/utils.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <limits>
#include <stdexcept>
#include <sstream>
#include <iostream>

using namespace strus;

#define EPSILON_FLOAT32 std::numeric_limits<float>::epsilon()

static bool compareFunctionEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT32 >= val2 && val1 <= val2 + EPSILON_FLOAT32);
}

static bool compareFunctionNotEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return !compareFunctionEqualFloat32( op1, op2);
}

static bool compareFunctionLessFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT32 < val2);
}

static bool compareFunctionLessEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 <= val2 + EPSILON_FLOAT32);
}

static bool compareFunctionGreaterFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 > val2 + EPSILON_FLOAT32);
}

static bool compareFunctionGreaterEqualFloat32( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT32 >= val2);
}

#define EPSILON_FLOAT16 0.0004887581f

static bool compareFunctionEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT16 >= val2 && val1 <= val2 + EPSILON_FLOAT16);
}

static bool compareFunctionNotEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return !compareFunctionEqualFloat16( op1, op2);
}

static bool compareFunctionLessFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT16 < val2);
}

static bool compareFunctionLessEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 <= val2 + EPSILON_FLOAT16);
}

static bool compareFunctionGreaterFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 > val2 + EPSILON_FLOAT16);
}

static bool compareFunctionGreaterEqualFloat16( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT16 >= val2);
}

static bool compareFunctionEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 == (int)op2;
}

static bool compareFunctionNotEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return !compareFunctionEqualInt( op1, op2);
}

static bool compareFunctionLessInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 < (int)op2;
}

static bool compareFunctionLessEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 <= (int)op2;
}

static bool compareFunctionGreaterInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 > (int)op2;
}

static bool compareFunctionGreaterEqualInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (int)op1 >= (int)op2;
}

static bool compareFunctionEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 == (unsigned int)op2;
}

static bool compareFunctionNotEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return !compareFunctionEqualUInt( op1, op2);
}

static bool compareFunctionLessUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 < (unsigned int)op2;
}

static bool compareFunctionLessEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 <= (unsigned int)op2;
}

static bool compareFunctionGreaterUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 > (unsigned int)op2;
}

static bool compareFunctionGreaterEqualUInt( const ArithmeticVariant& op1, const ArithmeticVariant& op2)
{
	return (unsigned int)op1 >= (unsigned int)op2;
}

MetaDataCompareOperation::CompareFunction MetaDataCompareOperation::getCompareFunction( const char* type, CompareOperator cmpop)
{
	if (utils::caseInsensitiveEquals( type, "float16"))
	{
		switch (cmpop)
		{
			case MetaDataRestrictionInterface::CompareLess:
				return &compareFunctionLessFloat16;
			case MetaDataRestrictionInterface::CompareLessEqual:
				return &compareFunctionLessEqualFloat16;
			case MetaDataRestrictionInterface::CompareEqual:
				return &compareFunctionEqualFloat16;
			case MetaDataRestrictionInterface::CompareNotEqual:
				return &compareFunctionNotEqualFloat16;
			case MetaDataRestrictionInterface::CompareGreater:
				return &compareFunctionGreaterFloat16;
			case MetaDataRestrictionInterface::CompareGreaterEqual:
				return &compareFunctionGreaterEqualFloat16;
		}
		throw strus::runtime_error( _TXT( "unknown meta data compare function"));
	}
	else if (utils::caseInsensitiveEquals( type, "float32"))
	{
		switch (cmpop)
		{
			case MetaDataRestrictionInterface::CompareLess:
				return &compareFunctionLessFloat32;
			case MetaDataRestrictionInterface::CompareLessEqual:
				return &compareFunctionLessEqualFloat32;
			case MetaDataRestrictionInterface::CompareEqual:
				return &compareFunctionEqualFloat32;
			case MetaDataRestrictionInterface::CompareNotEqual:
				return &compareFunctionNotEqualFloat32;
			case MetaDataRestrictionInterface::CompareGreater:
				return &compareFunctionGreaterFloat32;
			case MetaDataRestrictionInterface::CompareGreaterEqual:
				return &compareFunctionGreaterEqualFloat32;
		}
		throw strus::runtime_error( _TXT( "unknown meta data compare function"));
	}
	else if (utils::caseInsensitiveStartsWith( type, "int"))
	{
		switch (cmpop)
		{
			case MetaDataRestrictionInterface::CompareLess:
				return &compareFunctionLessInt;
			case MetaDataRestrictionInterface::CompareLessEqual:
				return &compareFunctionLessEqualInt;
			case MetaDataRestrictionInterface::CompareEqual:
				return &compareFunctionEqualInt;
			case MetaDataRestrictionInterface::CompareNotEqual:
				return &compareFunctionNotEqualInt;
			case MetaDataRestrictionInterface::CompareGreater:
				return &compareFunctionGreaterInt;
			case MetaDataRestrictionInterface::CompareGreaterEqual:
				return &compareFunctionGreaterEqualInt;
		}
		throw strus::runtime_error( _TXT( "unknown meta data compare function"));
	}
	else if (utils::caseInsensitiveStartsWith( type, "uint"))
	{
		switch (cmpop)
		{
			case MetaDataRestrictionInterface::CompareLess:
				return &compareFunctionLessUInt;
			case MetaDataRestrictionInterface::CompareLessEqual:
				return &compareFunctionLessEqualUInt;
			case MetaDataRestrictionInterface::CompareEqual:
				return &compareFunctionEqualUInt;
			case MetaDataRestrictionInterface::CompareNotEqual:
				return &compareFunctionNotEqualUInt;
			case MetaDataRestrictionInterface::CompareGreater:
				return &compareFunctionGreaterUInt;
			case MetaDataRestrictionInterface::CompareGreaterEqual:
				return &compareFunctionGreaterEqualUInt;
		}
		throw strus::runtime_error( _TXT( "unknown meta data compare function"));
	}
	else
	{
		throw strus::runtime_error( _TXT( "unknown type in meta data restriction: '%s'"), type);
	}
}

bool MetaDataCompareOperation::match( const MetaDataReaderInterface* md) const
{
	return m_func( md->getValue( m_elementHandle), m_operand);
}

MetaDataRestriction::MetaDataRestriction(
		const StorageClientInterface* storage,
		ErrorBufferInterface* errorhnd_)
	:m_metadata(storage->createMetaDataReader())
	,m_errorhnd(errorhnd_)
{
	if (!m_metadata.get())
	{
		throw strus::runtime_error( _TXT("failed to create metadata reader interface"));
	}
}

MetaDataRestriction::MetaDataRestriction(
		const Reference<MetaDataReaderInterface>& metadata_,
		ErrorBufferInterface* errorhnd_)
	:m_metadata(metadata_)
	,m_errorhnd(errorhnd_)
{}

bool MetaDataRestriction::match( const Index& docno) const
{
	m_metadata->skipDoc( docno);
	std::vector<MetaDataCompareOperation>::const_iterator
		oi = m_opar.begin(), oe = m_opar.end();
	while (oi != oe)
	{
		bool val = oi->match( m_metadata.get());
		for (++oi; oi != oe && !oi->m_newGroup; ++oi)
		{
			val |= oi->match( m_metadata.get());
		}
		if (!val) return false;
	}
	return true;
}

void MetaDataRestriction::addCondition(
		CompareOperator opr,
		const std::string& name,
		const ArithmeticVariant& operand,
		bool newGroup)
{
	try
	{
		Index elemhnd = m_metadata->elementHandle( name);
		if (elemhnd < 0)
		{
			m_errorhnd->explain( _TXT( "cannot create metadata restriction: %s"));
		}
		const char* elemtype = m_metadata->getType( elemhnd);
		if (!elemtype)
		{
			m_errorhnd->explain( _TXT( "cannot evaluate metadata restriction type: %s"));
		}
		m_opar.push_back( MetaDataCompareOperation( elemtype, opr, elemhnd, name, operand, newGroup));
	}
	CATCH_ERROR_MAP( _TXT("error in meta data restriction add condition: %s"), *m_errorhnd);

}

static const char* compareOperatorName( MetaDataRestrictionInterface::CompareOperator op)
{
	static const char* ar[] = {"<", "<=", "==", "!=", ">", ">="};
	return ar[op];
}

std::string MetaDataRestriction::tostring() const
{
	try
	{
		std::ostringstream resbuf;
		std::vector<MetaDataCompareOperation>::const_iterator
			oi = m_opar.begin(), oe = m_opar.end();
		if (oi == oe) return std::string();
		for (++oi; oi != oe && !oi->m_newGroup; ++oi){}
		bool has_groups = oi != oe;
		//... true if we have no two operand groups joined with 'and' (its only one big 'or' expression)
	
		oi = m_opar.begin(), oe = m_opar.end();
		while (oi != oe)
		{
			bool has_brackets = ((oi+1) != oe && !(oi+1)->m_newGroup);
			//... the 'or' group consists of one element only (no brackets needed)
			if (has_brackets && has_groups) resbuf << "(";
			// Print the first element of the 'or' group
			resbuf << oi->m_name << ' ' << compareOperatorName(oi->m_opr) << ' ' << oi->m_operand.tostring().c_str();
			for (++oi; oi != oe && !oi->m_newGroup; ++oi)
			{
				// Print the rest elements of the 'or' group
				resbuf << " or " << oi->m_name << ' ' << compareOperatorName(oi->m_opr) << ' ' << oi->m_operand.tostring().c_str();
			}
			if (has_brackets && has_groups) resbuf << ")";
			if (oi != oe) resbuf << " and ";
		}
		return resbuf.str();
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error in meta data restriction to string: %s"), *m_errorhnd, std::string());
}




