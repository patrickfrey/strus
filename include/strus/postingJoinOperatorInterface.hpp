/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Interface for a function implementing a join of N posting sets and providing an posting iterator on the result.
/// \file "postingJoinOperatorInterface.hpp"
#ifndef _STRUS_POSTING_JOIN_OPERATOR_INTERFACE_HPP_INCLUDED
#define _STRUS_POSTING_JOIN_OPERATOR_INTERFACE_HPP_INCLUDED
#include "strus/reference.hpp"
#include <vector>
#include <string>

namespace strus
{
/// \brief Forward declaration
class PostingIteratorInterface;

/// \brief Interface for creating iterators on joined sets of postings
class PostingJoinOperatorInterface
{
public:
	/// \brief Destructor
	virtual ~PostingJoinOperatorInterface(){}

	/// \brief Create an iterator on the join operator result (set of postings)
	/// \param[in] argitrs argument posting iterators of the join operation
	/// \param[in] range range of the operation
	/// \param[in] cardinality required size of matching results (e.g. minimum number of elements of any input subset selection that builds a result) (0 for default)
	/// \return the iterator on the resulting set of postings
	virtual PostingIteratorInterface* createResultIterator(
			const std::vector<Reference<PostingIteratorInterface> >& argitrs,
			int range,
			unsigned int cardinality) const=0;

	/// \brief Structure that describes the join operator
	struct Description
	{
		/// \brief Default constructor
		Description()
			:m_name(),m_text(){}
		/// \brief Constructor
		Description( const std::string& name_, const std::string& text_)
			:m_name(name_),m_text(text_){}
		/// \brief Copy constructor
		Description( const Description& o)
			:m_name(o.m_name),m_text(o.m_text){}

		/// \brief Get description text
		const std::string& name() const			{return m_name;}
		/// \brief Get description text
		const std::string& text() const			{return m_text;}

	private:
		std::string m_name;		///< name of the operator
		std::string m_text;		///< description text
	};

	/// \brief Get a description of the function for user help and introspection
	/// \return the description structure
	virtual Description getDescription() const=0;
};

}//namespace
#endif


