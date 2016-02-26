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
#ifndef _STRUS_METADATA_RESTRICTION_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_METADATA_RESTRICTION_IMPLEMENTATION_HPP_INCLUDED
#include "strus/metaDataRestrictionInterface.hpp"
#include "strus/arithmeticVariant.hpp"
#include "strus/index.hpp"
#include "strus/reference.hpp"
#include <vector>
#include <string>

namespace strus {

/// \brief Forward declaration
class StorageClientInterface;
/// \brief Forward declaration
class MetaDataReaderInterface;
/// \brief Forward declaration
class ErrorBufferInterface;

/// \brief Structure representing one compare operation in a meta data restriction
class MetaDataCompareOperation
{
public:
	/// \brief Comparison operator implemented
	typedef MetaDataRestrictionInterface::CompareOperator CompareOperator;
	/// \brief Function implementing the comparison operator
	typedef bool (*CompareFunction)( const ArithmeticVariant& op1, const ArithmeticVariant& op2);

	/// \brief Copy constructor
	MetaDataCompareOperation( const MetaDataCompareOperation& o)
		:m_opr(o.m_opr)
		,m_func(o.m_func)
		,m_elementHandle(o.m_elementHandle)
		,m_name(o.m_name)
		,m_operand(o.m_operand)
		,m_newGroup(o.m_newGroup){}

	/// \brief Constructor
	MetaDataCompareOperation(
			const char* type_,
			CompareOperator opr_,
			const Index& elementHandle_,
			const std::string& name_,
			const ArithmeticVariant& operand_,
			bool newGroup_)
		:m_opr(opr_)
		,m_func(getCompareFunction(type_,opr_))
		,m_elementHandle(elementHandle_)
		,m_name(name_)
		,m_operand(operand_)
		,m_newGroup(newGroup_){}

	/// \brief Try to match this condition
	/// \return true if yes
	bool match( const MetaDataReaderInterface* md) const;

private:
	static CompareFunction getCompareFunction( const char* type_, CompareOperator opr_);

private:
	friend class MetaDataRestriction;
	CompareOperator m_opr;		///< comparison operator id
	CompareFunction m_func;		///< comparison function implementing m_opr
	Index m_elementHandle;		///< metadata element handle
	std::string m_name;		///< name of metadata table element
	ArithmeticVariant m_operand;	///< operand to compare with the metadata table element
	bool m_newGroup;		///< true if element opens a new OR group in a CNF
};


class MetaDataRestriction
	:public MetaDataRestrictionInterface
{
public:
	MetaDataRestriction(
			const StorageClientInterface* storage,
			ErrorBufferInterface* errorhnd_);
	MetaDataRestriction(
			const Reference<MetaDataReaderInterface>& metadata_,
			ErrorBufferInterface* errorhnd_);

	virtual ~MetaDataRestriction(){}
	
	virtual void addCondition(
			CompareOperator opr,
			const std::string& name,
			const ArithmeticVariant& operand,
			bool newGroup);

	virtual std::string tostring() const;

	virtual bool match( const Index& docno) const;

private:
	std::vector<MetaDataCompareOperation> m_opar;		///< list of comparison operations as CNF
	mutable Reference<MetaDataReaderInterface> m_metadata;	///< we change it only when calling match and there is no other method accessing this metadata reader
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};

}
#endif

