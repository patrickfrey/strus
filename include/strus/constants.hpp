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
/// \brief Collection of constants that are shared between components of strus
/// \file "constants.hpp"
#ifndef _STRUS_GLOBAL_CONSTANTS_HPP_INCLUDED
#define _STRUS_GLOBAL_CONSTANTS_HPP_INCLUDED

namespace strus
{

/// \brief Some reserved global constants that document some dependencies (hacks) that did not get yet into interfaces (and probably never will).
struct Constants
{
	/// \brief Get the agreed attribute name for the document id
	/// \note The inserter program set this attribute implicitely to the value of the path of the document inserted
	static const char* attribute_docid()
	{
		return "docid";
	}
	/// \brief Get the name of the set union operator for postings
	/// \note The query evaluation uses implicitely this operator to make joins of posting sets that have to be merged before passing it to some function (like for example the structure element posting set passed to summarizers)
	static const char* operator_set_union()
	{
		return "union";
	}
	/// \brief Get the name of the set join operator for postings for query phrase terms that got the same position asigned
	/// \note The standard query language uses this operator to build query expressions for phrases in case of multiple occurrencies of distinct terms at the same position in the query
	static const char* operator_query_phrase_same_position()
	{
		return "intersect";
	}
	/// \brief Get the name of the set join operator for postings for query phrase terms that got ascending position assigned
	/// \note The standard query language uses this operator to build query expressions for phrases in case of subsequent occurrencies of terms in a query phrase
	static const char* operator_query_phrase_sequence()
	{
		return "sequence";
	}
	/// \brief Get the term type that does not exist and is reserved for the empty posting set
	static const char* query_empty_postings_termtype()
	{
		return "";
	}
	/// \brief Get the identifier used in protocols for distributing the document frequency
	static const char* storage_statistics_document_frequency()
	{
		return "df";
	}
	/// \brief Get the identifier used in protocols for distributing the collection size
	static const char* storage_statistics_number_of_documents()
	{
		return "dn";
	}

	/// \brief Get the maximum position (counted from 1) in a document a token can have 
	/// \note This is a limit given by the implementation of the position info block. Unfortunately it creeps through the system.
	static unsigned int storage_max_position_info()
	{
		return 65535;
	}
};

}//namespace
#endif
