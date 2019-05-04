/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
	/// \brief Get the attribute name for the document id
	/// \note The inserter program set this attribute implicitely to the value of the path of the document inserted
	static const char* attribute_docid()
	{
		return "docid";
	}
	/// \brief Get the name reserved for the internal document number
	static const char* identifier_docno()
	{
		return "docno";
	}
	/// \brief Get the name reserved for addressing ACLs (access control lists)
	static const char* identifier_acl()
	{
		return "acl";
	}
	/// \brief Get the name reserved for ordinal positions assigned to terms in a document
	static const char* identifier_position()
	{
		return "position";
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
	/// \brief Get the maximum position (counted from 1) in a document a token can have 
	/// \note This is a limit given by the implementation of the position info block. Unfortunately it creeps through the system.
	static unsigned int storage_max_position_info()
	{
		return 65535;
	}
	/// \brief Get the name of the leveldb database type used as default database type
	static const char* leveldb_database_name()
	{
		return "leveldb";
	}
	/// \brief Get the name of the standard (default) vector storage (naming the storage type in the storage configuration)
	static const char* standard_vector_storage()
	{
		return "vector_std";
	}
	/// \brief Get the name of the standard (default) vector storage module
	static const char* standard_vector_storage_module()
	{
		return "storage_vector_std";
	}
	/// \brief Get the name of the standard (default) pattern matcher
	static const char* standard_pattern_matcher()
	{
		return "std";
	}
	/// \brief Get the name of the standard (default) sentence analyzer
	static const char* standard_sentence_analyzer()
	{
		return "std";
	}
	/// \brief Get the name of the standard (default) statistics processor
	static const char* standard_statistics_processor()
	{
		return "std";
	}
	/// \brief Get the name of the standard (default) pattern match module
	static const char* standard_pattern_matcher_module()
	{
		return "analyzer_pattern";
	}
	/// \brief Get the default type feature separator in a word2vec vector file (where you have only a single identifier describing the item)
	static char standard_word2vec_type_feature_separator()
	{
		return '#';
	}
	/// \brief Get the default similarity distance for grouping similar elements in the query analyzer
	static double defaultGroupSimilarityDistance()
	{
		return 0.60;
	}
};

}//namespace
#endif
