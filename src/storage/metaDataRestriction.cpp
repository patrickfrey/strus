/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "metaDataRestriction.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/storageClientInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include "strus/base/string_conv.hpp"
#include <limits>
#include <stdexcept>
#include <sstream>
#include <iostream>

using namespace strus;

#define EPSILON_FLOAT32 std::numeric_limits<float>::epsilon()

static bool compareFunctionEqualFloat32( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT32 >= val2 && val1 <= val2 + EPSILON_FLOAT32);
}

static bool compareFunctionNotEqualFloat32( const NumericVariant& op1, const NumericVariant& op2)
{
	return !compareFunctionEqualFloat32( op1, op2);
}

static bool compareFunctionLessFloat32( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT32 < val2);
}

static bool compareFunctionLessEqualFloat32( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 <= val2 + EPSILON_FLOAT32);
}

static bool compareFunctionGreaterFloat32( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 > val2 + EPSILON_FLOAT32);
}

static bool compareFunctionGreaterEqualFloat32( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT32 >= val2);
}

#define EPSILON_FLOAT16 0.0004887581f

static bool compareFunctionEqualFloat16( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT16 >= val2 && val1 <= val2 + EPSILON_FLOAT16);
}

static bool compareFunctionNotEqualFloat16( const NumericVariant& op1, const NumericVariant& op2)
{
	return !compareFunctionEqualFloat16( op1, op2);
}

static bool compareFunctionLessFloat16( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT16 < val2);
}

static bool compareFunctionLessEqualFloat16( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 <= val2 + EPSILON_FLOAT16);
}

static bool compareFunctionGreaterFloat16( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 > val2 + EPSILON_FLOAT16);
}

static bool compareFunctionGreaterEqualFloat16( const NumericVariant& op1, const NumericVariant& op2)
{
	double val1 = op1;
	double val2 = op2;
	return (val1 + EPSILON_FLOAT16 >= val2);
}

static bool compareFunctionEqualInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.toint() == op2.toint();
}

static bool compareFunctionNotEqualInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return !compareFunctionEqualInt( op1, op2);
}

static bool compareFunctionLessInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.toint() < op2.toint();
}

static bool compareFunctionLessEqualInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.toint() <= op2.toint();
}

static bool compareFunctionGreaterInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.toint() > op2.toint();
}

static bool compareFunctionGreaterEqualInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.toint() >= op2.toint();
}

static bool compareFunctionEqualUInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.touint() == op2.touint();
}

static bool compareFunctionNotEqualUInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return !compareFunctionEqualUInt( op1, op2);
}

static bool compareFunctionLessUInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.touint() < op2.touint();
}

static bool compareFunctionLessEqualUInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.touint() <= op2.touint();
}

static bool compareFunctionGreaterUInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.touint() > op2.touint();
}

static bool compareFunctionGreaterEqualUInt( const NumericVariant& op1, const NumericVariant& op2)
{
	return op1.touint() >= op2.touint();
}

static const char* compareOperatorName( MetaDataRestrictionInterface::CompareOperator op)
{
	static const char* ar[] = {"<", "<=", "==", "!=", ">", ">="};
	return ar[op];
}

MetaDataCompareOperation::CompareFunction MetaDataCompareOperation::getCompareFunction( const char* type, CompareOperator cmpop)
{
	if (strus::caseInsensitiveEquals( type, "float16"))
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
		throw strus::runtime_error( "%s", _TXT( "unknown meta data compare function"));
	}
	else if (strus::caseInsensitiveEquals( type, "float32"))
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
		throw strus::runtime_error( "%s", _TXT( "unknown meta data compare function"));
	}
	else if (strus::caseInsensitiveStartsWith( type, "int"))
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
		throw strus::runtime_error( "%s", _TXT( "unknown meta data compare function"));
	}
	else if (strus::caseInsensitiveStartsWith( type, "uint"))
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
		throw strus::runtime_error( "%s", _TXT( "unknown meta data compare function"));
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

std::string MetaDataCompareOperation::tostring() const
{
	std::ostringstream rt;
	rt << "(" << m_name << compareOperatorName(m_opr) << m_operand.tostring().c_str()
		<< ")[" << (m_newGroup?"T":"F") << "]" << std::endl;
	return rt.str();
}

MetaDataRestrictionInstance::MetaDataRestrictionInstance(
		MetaDataReaderInterface* metadata_,
		const std::vector<MetaDataCompareOperation>& opar_,
		ErrorBufferInterface* errorhnd_)
	:m_opar(opar_)
	,m_metadata(metadata_)
	,m_errorhnd(errorhnd_)
{
	if (!m_metadata.get())
	{
		throw strus::runtime_error( "%s", _TXT("failed to create metadata reader interface"));
	}
}

bool MetaDataRestrictionInstance::match( const Index& docno) const
{
	m_metadata->skipDoc( docno);
	std::vector<MetaDataCompareOperation>::const_iterator
		oi = m_opar.begin(), oe = m_opar.end();
	while (oi != oe)
	{
		bool val = oi->match( m_metadata.get());
		for (++oi; oi != oe && !oi->newGroup(); ++oi)
		{
			val |= oi->match( m_metadata.get());
		}
		if (!val) return false;
	}
	return true;
}


MetaDataRestriction::MetaDataRestriction(
		const StorageClientInterface* storage_,
		ErrorBufferInterface* errorhnd_)
	:m_opar()
	,m_storage(storage_)
	,m_metadata(storage_->createMetaDataReader())
	,m_errorhnd(errorhnd_)
{}

void MetaDataRestriction::addCondition(
		const CompareOperator& opr,
		const std::string& name,
		const NumericVariant& operand,
		bool newGroup)
{
	try
	{
		Index elemhnd = m_metadata->elementHandle( name);
		if (elemhnd < 0)
		{
			throw strus::runtime_error( _TXT( "undefined metadata element: %s"), name.c_str());
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

MetaDataRestrictionInstanceInterface* MetaDataRestriction::createInstance() const
{
	try
	{
		return new MetaDataRestrictionInstance( m_storage->createMetaDataReader(), m_opar, m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("failed to create meta data restriction instance: %s"), *m_errorhnd, 0);
}



