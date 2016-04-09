/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_METADATA_RESTRICTION_IMPLEMENTATION_HPP_INCLUDED
#define _STRUS_METADATA_RESTRICTION_IMPLEMENTATION_HPP_INCLUDED
#include "strus/metaDataRestrictionInterface.hpp"
#include "strus/metaDataRestrictionInstanceInterface.hpp"
#include "strus/numericVariant.hpp"
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
	typedef bool (*CompareFunction)( const NumericVariant& op1, const NumericVariant& op2);

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
			const NumericVariant& operand_,
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

	/// \brief Return string representation of this operation
	std::string tostring() const;

	/// \brief Evaluate, if this element start a new CNF group
	/// \return true if yes
	bool newGroup() const					{return m_newGroup;}

private:
	static CompareFunction getCompareFunction( const char* type_, CompareOperator opr_);

private:
	friend class MetaDataRestriction;
	CompareOperator m_opr;		///< comparison operator id
	CompareFunction m_func;		///< comparison function implementing m_opr
	Index m_elementHandle;		///< metadata element handle
	std::string m_name;		///< name of metadata table element
	NumericVariant m_operand;	///< operand to compare with the metadata table element
	bool m_newGroup;		///< true if element opens a new OR group in a CNF
};

class MetaDataRestrictionInstance
	:public MetaDataRestrictionInstanceInterface
{
public:
	/// \param[in] metadata_ metadata reader (ownership passed)
	MetaDataRestrictionInstance(
			MetaDataReaderInterface* metadata_,
			const std::vector<MetaDataCompareOperation>& opar_,
			ErrorBufferInterface* errorhnd_);

	virtual ~MetaDataRestrictionInstance(){}

	virtual bool match( const Index& docno) const;

private:
	std::vector<MetaDataCompareOperation> m_opar;		///< list of comparison operations as CNF
	mutable Reference<MetaDataReaderInterface> m_metadata;	///< we change it only when calling match and there is no other method accessing this metadata reader
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};


class MetaDataRestriction
	:public MetaDataRestrictionInterface
{
public:
	MetaDataRestriction(
			const StorageClientInterface* storage_,
			ErrorBufferInterface* errorhnd_);

	virtual ~MetaDataRestriction()
	{}
	
	virtual void addCondition(
			const CompareOperator& opr,
			const std::string& name,
			const NumericVariant& operand,
			bool newGroup);

	virtual std::string tostring() const;

	virtual MetaDataRestrictionInstanceInterface* createInstance() const;

private:
	std::vector<MetaDataCompareOperation> m_opar;		///< list of comparison operations as CNF
	const StorageClientInterface* m_storage;		///< storage reference
	Reference<MetaDataReaderInterface> m_metadata;		///< meta data reader for inspecting the table elements
	ErrorBufferInterface* m_errorhnd;			///< buffer for reporting errors
};

}
#endif

