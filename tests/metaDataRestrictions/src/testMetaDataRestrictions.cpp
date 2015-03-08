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
#include "metaDataRestriction.hpp"
#include "metaDataDescription.hpp"
#include "metaDataElement.hpp"
#include "metaDataRecord.hpp"
#include "strus/metaDataReaderInterface.hpp"
#include "strus/arithmeticVariant.hpp"
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <limits>

#define RANDINT(MIN,MAX) ((rand()%(MAX-MIN))+MIN)

#undef STRUS_LOWLEVEL_DEBUG

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

	virtual strus::ArithmeticVariant getValue( const strus::Index& elementHandle_) const
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

private:
	const strus::MetaDataDescription* m_description;
	strus::MetaDataRecord m_current;
};

static strus::MetaDataDescription randomMetaDataDescription()
{
	std::size_t notFields = RANDINT(1,8);
	strus::MetaDataDescription rt;
	std::size_t ii=0;
	for (; ii<notFields; ++ii)
	{
		std::string name;
		name.push_back( 'A' + ii);
		strus::MetaDataElement::Type type = (strus::MetaDataElement::Type)RANDINT(0,strus::MetaDataElement::NofTypes);
		rt.add( type, name);
	}
	return rt;
}

static strus::MetaDataRecord randomMetaDataRecord(
		const strus::MetaDataDescription* descr, void* ptr)
{
	strus::MetaDataRecord rt( descr, ptr);
	std::vector<std::string> columns = descr->columns();
	std::vector<std::string>::const_iterator ci = columns.begin(), ce = columns.end();

	for (; ci != ce; ++ci)
	{
		strus::ArithmeticVariant val;
		strus::Index eh = descr->getHandle( *ci);
		const strus::MetaDataElement* elem = descr->get(eh);
		switch (elem->type())
		{
			case strus::MetaDataElement::Int8:
				val = strus::ArithmeticVariant((int)RANDINT(0,0xfFU)-0x7f);
				break;
			case strus::MetaDataElement::UInt8:
				val = strus::ArithmeticVariant( (unsigned int)RANDINT(0,0xfFU));
				break;
			case strus::MetaDataElement::Int16:
				val = strus::ArithmeticVariant( (int)RANDINT(0,0xffFFU)-0x7fFFU);
				break;
			case strus::MetaDataElement::UInt16:
				val = strus::ArithmeticVariant( (int)RANDINT(0,0xffFFU));
				break;
			case strus::MetaDataElement::Int32:
				val = strus::ArithmeticVariant( (int)(RANDINT(0,0xffffFFFFUL) - 0x7fffFFFFUL));
				break;
			case strus::MetaDataElement::UInt32:
				val = strus::ArithmeticVariant( (int)(RANDINT(0,0xffffFFFFUL)));
				break;
			case strus::MetaDataElement::Float16:
				val = strus::ArithmeticVariant( (float)RANDINT(0,0xffffFFFFUL)/RANDINT(1,0xffffFFFFUL));
				break;
			case strus::MetaDataElement::Float32:
				val = strus::ArithmeticVariant( (float)RANDINT(0,0xffffFFFFUL)/RANDINT(1,0xffffFFFFUL));
				break;
		}
		rt.setValue( elem, val);
	}
	return rt;
}

static int randomOfs( strus::QueryInterface::CompareOperator opr, bool positiveResult)
{
	if (!positiveResult)
	{
		switch (opr)
		{
			case strus::QueryInterface::CompareLess:
				opr = strus::QueryInterface::CompareGreaterEqual;
				break;
			case strus::QueryInterface::CompareLessEqual:
				opr = strus::QueryInterface::CompareGreater;
				break;
			case strus::QueryInterface::CompareEqual:
				opr = strus::QueryInterface::CompareNotEqual;
				break;
			case strus::QueryInterface::CompareNotEqual:
				opr = strus::QueryInterface::CompareEqual;
				break;
			case strus::QueryInterface::CompareGreater:
				opr = strus::QueryInterface::CompareLessEqual;
				break;
			case strus::QueryInterface::CompareGreaterEqual:
				opr = strus::QueryInterface::CompareLess;
				break;
		}
	}
	int ofs = RANDINT(1,100);
	switch (opr)
	{
		case strus::QueryInterface::CompareLess:
			break;
		case strus::QueryInterface::CompareLessEqual:
			if (ofs > 60)
			{
				ofs = 0;
			}
			break;
		case strus::QueryInterface::CompareEqual:
			ofs = 0;
			break;
		case strus::QueryInterface::CompareNotEqual:
			break;
		case strus::QueryInterface::CompareGreater:
			ofs = -ofs;
			break;
		case strus::QueryInterface::CompareGreaterEqual:
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

static strus::ArithmeticVariant randomOperand(
	const strus::ArithmeticVariant& baseValue,
	strus::QueryInterface::CompareOperator opr,
	strus::MetaDataElement::Type elemtype,
	bool positiveResult)
{
	int ofs = randomOfs( opr, positiveResult);
	switch (baseValue.type)
	{
		case strus::ArithmeticVariant::Null:
			return strus::ArithmeticVariant();
		case strus::ArithmeticVariant::Int:
		{
			int value = (int)baseValue;
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
			return strus::ArithmeticVariant( value + ofs);
		}
		case strus::ArithmeticVariant::UInt:
		{
			unsigned int value = (unsigned int)baseValue;
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
				return strus::ArithmeticVariant( value - (unsigned int)-ofs);
			}
			else
			{
				return strus::ArithmeticVariant( value + (unsigned int)ofs);
			}
		}
		case strus::ArithmeticVariant::Float:
		{
			float epsilon = std::numeric_limits<float>::epsilon();
			if (elemtype == strus::MetaDataElement::Float16)
			{
				epsilon = 0.0004887581f;
			}
			float value = (float)baseValue;
			unsigned int rd = RANDINT(1,5);
			if (ofs > 0)
			{
				value += epsilon/rd;
			}
			else if (ofs < 0)
			{
				value -= epsilon/rd;
			}
			else if (RANDINT(0,2) == 1)
			{
				value += epsilon/rd;
			}
			else
			{
				value -= epsilon/rd;
			}
			return strus::ArithmeticVariant( value + ofs * epsilon * RANDINT(1,4));
		}
	}
	return strus::ArithmeticVariant();
}

static strus::MetaDataRestriction randomMetaDataRestriction(
		const strus::MetaDataDescription* descr,
		const strus::MetaDataRecord& rec,
		bool newGroup,
		bool positiveResult)
{
	strus::Index hnd = RANDINT( 0, descr->nofElements());
	strus::QueryInterface::CompareOperator 
		opr = (strus::QueryInterface::CompareOperator)
			RANDINT( 0, strus::QueryInterface::NofCompareOperators);

	const strus::MetaDataElement* elem = descr->get( hnd);
	strus::ArithmeticVariant value = rec.getValue( elem);

	const char* typeName = elem->typeName();

	strus::ArithmeticVariant 
		operand = randomOperand( value, opr, elem->type(), positiveResult);
	
	return strus::MetaDataRestriction( typeName, opr, hnd, operand, newGroup);
}

static std::vector<strus::MetaDataRestriction> randomMetaDataRestrictionList(
		const strus::MetaDataDescription* descr,
		const strus::MetaDataRecord& rec,
		bool positiveResult)
{		
	std::vector<strus::MetaDataRestriction> rt;
	unsigned int ri = 0, re = RANDINT(1,6);
	for (; ri != re; ++ri)
	{
		unsigned int nofGroupElements = RANDINT(1,6);
		if (nofGroupElements > 3)
		{
			nofGroupElements = 1;
		}
		unsigned int matchGroupElement = RANDINT( 0, nofGroupElements);
		unsigned int gi = 0, ge = nofGroupElements;
		for (; gi != ge; ++gi)
		{
			bool positiveResultGroupElement = positiveResult?(gi==matchGroupElement):false;
			bool newGroup = (gi == 0);
			rt.push_back(
				randomMetaDataRestriction(
					descr, rec, newGroup, positiveResultGroupElement));
		}
	}
	return rt;
}

static void reportTest(
		std::ostream& out,
		const std::vector<strus::MetaDataRestriction>& restrictions,
		const strus::MetaDataRecord& rc,
		bool expectedResult)
{
	out << "checking record:" << std::endl;
	rc.print( out);
	out << "against expression:" << std::endl;
	std::vector<strus::MetaDataRestriction>::const_iterator
		ri = restrictions.begin(), re = restrictions.end();
	for (; ri != re; ++ri)
	{
		ri->print( out);
	}
	out << "expecting " << (expectedResult?"positive":"negative")
			<< " result" << std::endl;
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
				MetaDataReader reader( &descr, data);

				std::size_t xi=0, xe=nofQueries;
				for (; xi<xe; ++xi)
				{
					try
					{
						bool expectedResult = (bool)RANDINT(0,2);
						std::vector<strus::MetaDataRestriction>
							restrictions = randomMetaDataRestrictionList(
										&descr, rc, expectedResult);

#ifdef STRUS_LOWLEVEL_DEBUG
						std::cerr << "test " << di << "/" << ri << "/" << xi << std::endl;
						reportTest( std::cerr, restrictions, rc, expectedResult);
#endif
						queryCount++;
						if (expectedResult != strus::matchesMetaDataRestriction(
									restrictions, &reader))
						{
#ifndef STRUS_LOWLEVEL_DEBUG
							reportTest( std::cerr, restrictions, rc, expectedResult);
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
		std::cerr << "OK processed " << (nofTables * nofRecords * nofQueries) << " (" << ((nofTables * nofRecords * nofQueries) + failedRandomQueries) << ") random meta data table queries with success" << std::endl;
		return 0;
	}
	catch (const std::exception& err)
	{
		std::cerr << "EXCEPTION " << err.what() << std::endl;
	}
	return -1;
}


