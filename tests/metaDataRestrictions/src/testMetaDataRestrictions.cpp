/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "metaDataRestriction.hpp"
#include "metaDataDescription.hpp"
#include "metaDataElement.hpp"
#include "metaDataRecord.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/metaDataRestrictionInterface.hpp"
#include "strus/numericVariant.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/lib/error.hpp"
#include "strus/base/stdint.h"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>

static strus::Reference<strus::ErrorBufferInterface> g_errorbuf;

static void initRand()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	::srand( ((now->tm_year+1) * (now->tm_mon+100) * (now->tm_mday+1)));
}

static int32_t randint( int32_t mi, int32_t me)
{
	return (int32_t)((int64_t)rand() % ((int64_t)me - (int64_t)mi) + (int64_t)mi);
}

static uint32_t randuint( uint32_t mi, uint32_t me)
{
	return (int32_t)((uint32_t)rand() % (me - mi) + mi);
}

#define STRUS_LOWLEVEL_DEBUG

class MetaDataReader
	:public strus::MetaDataReaderInterface
{
public:
	MetaDataReader( const strus::MetaDataDescription* description_, void* data_)
		:m_description(description_),m_current(description_,data_){}

	virtual strus::Index elementHandle( const std::string& name) const
	{
		return m_description->getHandle( name);
	}

	virtual bool hasElement( const std::string& name) const
	{
		return m_description->hasElement( name);
	}

	virtual void skipDoc( const strus::Index&)
	{}

	virtual strus::NumericVariant getValue( const strus::Index& elementHandle_) const
	{
		return m_current.getValue( m_description->get( elementHandle_));
	}

	virtual const char* getType( const strus::Index& elementHandle_) const
	{
		return m_description->get( elementHandle_)->typeName();
	}

	virtual const char* getName( const strus::Index& elementHandle_) const
	{
		return m_description->getName( elementHandle_);
	}

	virtual strus::Index nofElements() const
	{
		return (strus::Index)m_description->nofElements();
	}

	std::vector<std::string> getNames() const
	{
		return m_description->columns();
	}
	

private:
	const strus::MetaDataDescription* m_description;
	strus::MetaDataRecord m_current;
};

static strus::MetaDataDescription randomMetaDataDescription()
{
	std::size_t notFields = randuint(1,8);
	strus::MetaDataDescription rt;
	std::size_t ii=0;
	for (; ii<notFields; ++ii)
	{
		std::string name;
		name.push_back( 'A' + ii);
		strus::MetaDataElement::Type type = (strus::MetaDataElement::Type)randuint(0,strus::MetaDataElement::NofTypes);
		rt.add( type, name);
	}
	return rt;
}

static bool isValueEqual( strus::MetaDataElement::Type type, const strus::NumericVariant& aa, const strus::NumericVariant& bb)
{
	if (aa.type != bb.type) return false;
	if (type == strus::MetaDataElement::Float16)
	{
		if (aa.type != strus::NumericVariant::Float) return false;
		double diff = aa.tofloat() - bb.tofloat();
		if (diff < 0.0) diff = -diff;
		/// [PF:HACK] Bad epsilon !
		double epsilon = std::max<double>( 0.002 * aa.tofloat(), 0.002);
		if (epsilon < 0.0) epsilon = -epsilon;
		return (diff <= epsilon);
	}
	else if (type == strus::MetaDataElement::Float32)
	{
		if (aa.type != strus::NumericVariant::Float) return false;
		double diff = aa.tofloat() - bb.tofloat();
		if (diff < 0.0) diff = -diff;
		/// [PF:HACK] Bad epsilon !
		double epsilon = std::max<double>( 0.000001 * aa.tofloat(), 0.000001);
		if (epsilon < 0.0) epsilon = -epsilon;
		return (diff <= epsilon);
	}
	else
	{
		return aa == bb;
	}
}

static strus::MetaDataRecord randomMetaDataRecord(
		const strus::MetaDataDescription* descr, void* ptr)
{
	strus::MetaDataRecord rt( descr, ptr);
	std::vector<std::string> columns = descr->columns();
	std::vector<std::string>::const_iterator ci = columns.begin(), ce = columns.end();
	std::vector<strus::NumericVariant> valar;

	for (; ci != ce; ++ci)
	{
		strus::NumericVariant val;
		strus::Index eh = descr->getHandle( *ci);
		const strus::MetaDataElement* elem = descr->get(eh);
		switch (elem->type())
		{
			case strus::MetaDataElement::Int8:
				val = strus::NumericVariant( (strus::NumericVariant::IntType)(randint( std::numeric_limits<int8_t>::min(),std::numeric_limits<int8_t>::max())));
				break;
			case strus::MetaDataElement::UInt8:
				val = strus::NumericVariant( (strus::NumericVariant::UIntType)randuint(0,std::numeric_limits<uint8_t>::max()));
				break;
			case strus::MetaDataElement::Int16:
				val = strus::NumericVariant( (strus::NumericVariant::IntType)(randint( std::numeric_limits<int16_t>::min(),std::numeric_limits<int16_t>::max())));
				break;
			case strus::MetaDataElement::UInt16:
				val = strus::NumericVariant( (strus::NumericVariant::UIntType)randuint(0,std::numeric_limits<uint16_t>::max()));
				break;
			case strus::MetaDataElement::Int32:
				val = strus::NumericVariant( (strus::NumericVariant::IntType)(randint( std::numeric_limits<int32_t>::min(),std::numeric_limits<int32_t>::max())));
				break;
			case strus::MetaDataElement::UInt32:
				val = strus::NumericVariant( (strus::NumericVariant::UIntType)(randuint(0,std::numeric_limits<uint32_t>::max())));
				break;
			case strus::MetaDataElement::Float16:
				val = strus::NumericVariant( (double)randuint(0,0xffffFFFFUL)/(double)randuint(1,0xffffFFFFUL));
				break;
			case strus::MetaDataElement::Float32:
				val = strus::NumericVariant( (double)randuint(0,0xffffFFFFUL)/(double)randuint(1,0xffffFFFFUL));
				break;
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		strus::NumericVariant::String valstr( val);
		std::cerr << "[" << eh << "] " << elem->typeName() << " " << descr->getName( eh) << " = " << valstr << std::endl;
#endif
		rt.setValue( elem, val);
		valar.push_back( val);
	}
	std::vector<strus::NumericVariant>::const_iterator vi = valar.begin(), ve = valar.end();
	int cidx = 0;
	for (ci = columns.begin(); ci != ce && vi != ve; ++ci,++vi,++cidx)
	{
		strus::Index eh = descr->getHandle( *ci);
		const strus::MetaDataElement* elem = descr->get(eh);
		strus::NumericVariant val( rt.getValue( elem));
#ifdef STRUS_LOWLEVEL_DEBUG
		strus::NumericVariant::String valstr( val);
		strus::NumericVariant::String vistr( *vi);
		std::cerr << "[" << cidx << "] check value " << valstr.c_str() << " == " << vistr.c_str() << std::endl;
#endif
		if (!isValueEqual( elem->type(), val, *vi))
		{
			throw std::runtime_error("meta data record value is not stored as expected");
		}
	}
	return rt;
}

static int randomOfs( strus::MetaDataRestrictionInterface::CompareOperator opr, bool positiveResult)
{
	if (!positiveResult)
	{
		switch (opr)
		{
			case strus::MetaDataRestrictionInterface::CompareLess:
				opr = strus::MetaDataRestrictionInterface::CompareGreaterEqual;
				break;
			case strus::MetaDataRestrictionInterface::CompareLessEqual:
				opr = strus::MetaDataRestrictionInterface::CompareGreater;
				break;
			case strus::MetaDataRestrictionInterface::CompareEqual:
				opr = strus::MetaDataRestrictionInterface::CompareNotEqual;
				break;
			case strus::MetaDataRestrictionInterface::CompareNotEqual:
				opr = strus::MetaDataRestrictionInterface::CompareEqual;
				break;
			case strus::MetaDataRestrictionInterface::CompareGreater:
				opr = strus::MetaDataRestrictionInterface::CompareLessEqual;
				break;
			case strus::MetaDataRestrictionInterface::CompareGreaterEqual:
				opr = strus::MetaDataRestrictionInterface::CompareLess;
				break;
		}
	}
	int ofs = randuint(1,100);
	switch (opr)
	{
		case strus::MetaDataRestrictionInterface::CompareLess:
			break;
		case strus::MetaDataRestrictionInterface::CompareLessEqual:
			if (ofs > 60)
			{
				ofs = 0;
			}
			break;
		case strus::MetaDataRestrictionInterface::CompareEqual:
			ofs = 0;
			break;
		case strus::MetaDataRestrictionInterface::CompareNotEqual:
			break;
		case strus::MetaDataRestrictionInterface::CompareGreater:
			ofs = -ofs;
			break;
		case strus::MetaDataRestrictionInterface::CompareGreaterEqual:
			if (ofs > 60)
			{
				ofs = 0;
			}
			ofs = -ofs;
			break;
	}
	return ofs;
}

struct RandomDataException
{};

static strus::NumericVariant randomOperand(
	const strus::NumericVariant& baseValue,
	strus::MetaDataRestrictionInterface::CompareOperator opr,
	strus::MetaDataElement::Type elemtype,
	bool positiveResult)
{
	int ofs = randomOfs( opr, positiveResult);
	switch (baseValue.type)
	{
		case strus::NumericVariant::Null:
			return strus::NumericVariant();
		case strus::NumericVariant::Int:
		{
			int value = baseValue.toint();
			if (ofs < 0)
			{
				if (-ofs > value)
				{
					throw RandomDataException();
				}
			}
			else if (value > 0 && ofs + value <= 0)
			{
				throw RandomDataException();
			}
			return strus::NumericVariant( (strus::NumericVariant::IntType)value + ofs);
		}
		case strus::NumericVariant::UInt:
		{
			unsigned int value = baseValue.touint();
			if (ofs < 0)
			{
				if ((unsigned int)-ofs > value)
				{
					throw RandomDataException();
				}
			}
			else if ((unsigned int)ofs + value <= 0)
			{
				throw RandomDataException();
			}
			if (ofs < 0)
			{
				return strus::NumericVariant( (strus::NumericVariant::UIntType)value - (strus::NumericVariant::UIntType)-ofs);
			}
			else
			{
				return strus::NumericVariant( (strus::NumericVariant::UIntType)value + (strus::NumericVariant::UIntType)ofs);
			}
		}
		case strus::NumericVariant::Float:
		{
			float epsilon = std::numeric_limits<float>::epsilon();
			if (elemtype == strus::MetaDataElement::Float16)
			{
				epsilon = 0.0004887581f;
			}
			double value = (double)baseValue;
			unsigned int rd = randuint(1,5);
			if (ofs > 0)
			{
				value += epsilon/rd;
			}
			else if (ofs < 0)
			{
				value -= epsilon/rd;
			}
			else if (randuint(0,2) == 1)
			{
				value += epsilon/rd;
			}
			else
			{
				value -= epsilon/rd;
			}
			return strus::NumericVariant( value + ofs * epsilon * randuint(1,4));
		}
	}
	return strus::NumericVariant();
}

static strus::MetaDataCompareOperation
	randomMetaDataCondition(
		const strus::MetaDataDescription* descr,
		const strus::MetaDataRecord& rec,
		bool newGroup,
		bool positiveResult)
{
	strus::Index hnd = randuint( 0, descr->nofElements());
	strus::MetaDataRestrictionInterface::CompareOperator 
		opr = (strus::MetaDataRestrictionInterface::CompareOperator)
			randuint( 0, strus::MetaDataRestrictionInterface::NofCompareOperators);

	const strus::MetaDataElement* elem = descr->get( hnd);
	strus::NumericVariant value = rec.getValue( elem);

	std::string elemName( descr->getName( hnd));
	strus::NumericVariant 
		operand = randomOperand( value, opr, elem->type(), positiveResult);

	return strus::MetaDataCompareOperation(
			elem->typeName(), opr, hnd, elemName, operand, newGroup);
}

static strus::Reference<strus::MetaDataRestrictionInstanceInterface>
	randomMetaDataRestriction(
		strus::MetaDataReaderInterface* reader,
		const strus::MetaDataDescription* descr,
		const strus::MetaDataRecord& rec,
		bool positiveResult,
		std::string& expressionstr)
{
	std::vector<strus::MetaDataCompareOperation> ops;
	unsigned int ri = 0, re = randuint(1,6);
	for (; ri != re; ++ri)
	{
		unsigned int nofGroupElements = randuint(1,6);
		if (nofGroupElements > 3)
		{
			nofGroupElements = 1;
		}
		unsigned int matchGroupElement = randuint( 0, nofGroupElements);
		unsigned int gi = 0, ge = nofGroupElements;
		for (; gi != ge; ++gi)
		{
			bool positiveResultGroupElement = positiveResult?(gi==matchGroupElement):false;
			bool newGroup = (gi == 0);
			ops.push_back( randomMetaDataCondition( descr, rec, newGroup, positiveResultGroupElement));
			if (expressionstr.size())
			{
				expressionstr.append( ", ");
			}
			expressionstr.append( ops.back().tostring());
		}
	}
	strus::Reference<strus::MetaDataRestrictionInstanceInterface>
		rt( new strus::MetaDataRestrictionInstance( reader, ops, g_errorbuf.get()));
	return rt;
}

static void reportTest(
		std::ostream& out,
		const strus::Reference<strus::MetaDataRestrictionInstanceInterface>& restriction,
		const std::string& expressionstr,
		const strus::MetaDataRecord& rc,
		bool expectedResult)
{
	out << "checking record:" << std::endl;
	rc.print( out);
	out << "against expression:" << expressionstr << std::endl;
	out << "expecting " << (expectedResult?"positive":"negative") << " result" << std::endl;
}

static unsigned int getUintValue( const char* arg)
{
	unsigned int rt = 0, prev = 0;
	char const* cc = arg;
	for (; *cc; ++cc)
	{
		if (*cc < '0' || *cc > '9') throw std::runtime_error( std::string( "parameter is not a non negative integer number: ") + arg);
		rt = (rt * 10) + (*cc - '0');
		if (rt < prev) throw std::runtime_error( std::string( "parameter out of range: ") + arg);
	}
	return rt;
}

static void printUsage( int argc, const char* argv[])
{
	std::cerr << "usage: " << argv[0] << " <noftables> <nofrecords> <nofqueries>" << std::endl;
	std::cerr << "<noftables>  = number of random table descriptions to generate" << std::endl;
	std::cerr << "<nofrecords> = number of test meta data to generate for each table" << std::endl;
	std::cerr << "<nofqueries> = number of test queries to generate for each record" << std::endl;
}

int main( int argc, const char* argv[])
{
	g_errorbuf.reset( strus::createErrorBuffer_standard( stderr, 1));

	if (argc <= 1 || std::strcmp( argv[1], "-h") == 0 || std::strcmp( argv[1], "--help") == 0)
	{
		printUsage( argc, argv);
		return 0;
	}
	else if (argc < 4)
	{
		std::cerr << "ERROR too few parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	else if (argc > 4)
	{
		std::cerr << "ERROR too many parameters" << std::endl;
		printUsage( argc, argv);
		return 1;
	}
	try
	{
		unsigned int nofTables = getUintValue( argv[1]);
		unsigned int nofRecords = getUintValue( argv[2]);
		unsigned int nofQueries = getUintValue( argv[3]);
		unsigned int failedRandomQueries = 0;
		unsigned int queryCount = 0;

		initRand();
		std::size_t di=0, de=nofTables;
		for (; di<de; ++di)
		{
			strus::MetaDataDescription descr = randomMetaDataDescription();
			char data[ 1024];
			
			std::size_t ri=0, re=nofRecords;
			for (; ri<re; ++ri)
			{
				std::memset( data, 0, descr.bytesize());
				strus::MetaDataRecord rc = randomMetaDataRecord( &descr, data);

				std::size_t xi=0, xe=nofQueries;
				for (; xi<xe; ++xi)
				{
					try
					{
						std::string expressionstr;
						bool expectedResult = (bool)randuint(0,2);
						strus::Reference<strus::MetaDataRestrictionInstanceInterface>
							restriction =
								randomMetaDataRestriction(
									new MetaDataReader( &descr, data),
									&descr, rc, expectedResult, expressionstr);

#ifdef STRUS_LOWLEVEL_DEBUG
						std::cerr << "test " << di << "/" << ri << "/" << xi << std::endl;
						reportTest( std::cerr, restriction, expressionstr, rc, expectedResult);
#endif
						queryCount++;
						if (expectedResult != restriction->match(1))
						{
#ifndef STRUS_LOWLEVEL_DEBUG
							reportTest( std::cerr, restriction, expressionstr, rc, expectedResult);
#endif
							std::cout << "query no " << queryCount << " failed" << std::endl;
							throw std::runtime_error( "test failed");
						}
					}
					catch (const RandomDataException&)
					{
#ifdef STRUS_LOWLEVEL_DEBUG
						std::cerr << "could not generate test. trying again" << std::endl;
#endif
						failedRandomQueries += 1;
						--xi;
						continue;
					}
				}
			}
		}
		if (g_errorbuf->hasError())
		{
			std::cerr << "ERROR in metadata restriction test: %s" << g_errorbuf->fetchError() << std::endl;
		}
		else
		{
			std::cerr << "OK processed " << (nofTables * nofRecords * nofQueries) << " (" << ((nofTables * nofRecords * nofQueries) + failedRandomQueries) << ") random meta data table queries with success" << std::endl;
		}
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


