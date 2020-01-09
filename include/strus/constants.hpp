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
	static inline const char* attribute_docid()
	{
		return "docid";
	}
	/// \brief Get the attribute name for the access list of a document
	static inline const char* attribute_access()
	{
		return "access";
	}
	/// \brief Get the name reserved for the internal document number
	static inline const char* identifier_docno()
	{
		return "docno";
	}
	/// \brief Get the name reserved for addressing ACLs (access control lists)
	static inline const char* identifier_acl()
	{
		return "acl";
	}
	/// \brief Get the name reserved for ordinal positions assigned to terms in a document
	static inline const char* identifier_position()
	{
		return "position";
	}
	/// \brief Get the name of the set union operator for postings
	/// \note The query evaluation uses implicitely this operator to make joins of posting sets that have to be merged before passing it to some function (like for example the structure element posting set passed to summarizers)
	static inline const char* operator_set_union()
	{
		return "union";
	}
	/// \brief Get the name of the set join operator for postings for query phrase terms that got the same position asigned
	/// \note The standard query language uses this operator to build query expressions for phrases in case of multiple occurrencies of distinct terms at the same position in the query
	static inline const char* operator_query_phrase_same_position()
	{
		return "intersect";
	}
	/// \brief Get the name of the set join operator for postings for query phrase terms that got ascending position assigned
	/// \note The standard query language uses this operator to build query expressions for phrases in case of subsequent occurrencies of terms in a query phrase
	static inline const char* operator_query_phrase_sequence()
	{
		return "sequence";
	}
	/// \brief Get the term type that does not exist and is reserved for the empty posting set
	static inline const char* query_empty_postings_termtype()
	{
		return "";
	}
	/// \brief Get the maximum ordinal position (counted from 1) in a document a token can have 
	/// \note This is a limit given by the implementation of the position info block. Unfortunately it creeps through the system.
	/// \note The length of a novel is at least 50'000 words, "War and Peace" from Tolstoi has a word count of about 250'000. But it is controversial if it is sensible to have retrievable items of this size. For books I would rather suggest to have chapters as items with some meta data about the book as a whole included.
	static inline int storage_max_position_info()
	{
		return 65535;
	}
	/// \brief Get the mamimum number of levels for structures, determines how structure fields can overlap
	static inline int storage_max_struct_levels()
	{
		return 8;
	}
	/// \brief Get the mamimum number of links for structure fields
	static inline int storage_max_struct_field_links()
	{
		return 4;
	}
	/// \brief Defines the minimum value the fraction of the document frequency related to the collection size a feature must have to be considered as a "stopword". The categorization as stopword leads to some decicions about algorithms or data structures to use for the term.
	static inline float stopwordDfFactor()
	{
		return 0.1;
	}

	/// \brief Get the name of the leveldb database type used as default database type
	static inline const char* leveldb_database_name()
	{
		return "leveldb";
	}
	/// \brief Get the name of the standard (default) document length metadata element for weighting and summarizer functions relying on a notion of document length
	static inline const char* standard_metadata_document_length()
	{
		return "doclen";
	}
	/// \brief Get the name of the standard (default) vector storage (naming the storage type in the storage configuration)
	static inline const char* standard_vector_storage()
	{
		return "vector_std";
	}
	/// \brief Get the name of the standard (default) vector storage module
	static inline const char* standard_vector_storage_module()
	{
		return "storage_vector_std";
	}
	/// \brief Get the name of the standard (default) pattern matcher
	static inline const char* standard_pattern_matcher()
	{
		return "std";
	}
	/// \brief Get the name of the standard (default) sentence analyzer
	static inline const char* standard_sentence_analyzer()
	{
		return "std";
	}
	/// \brief Get the name of the standard (default) statistics processor
	static inline const char* standard_statistics_processor()
	{
		return "std";
	}
	/// \brief Get the name of the standard (default) pattern match module
	static inline const char* standard_pattern_matcher_module()
	{
		return "analyzer_pattern";
	}
	/// \brief Get the default type feature separator in a word2vec vector file (where you have only a single identifier describing the item)
	static inline char standard_word2vec_type_feature_separator()
	{
		return '#';
	}
	/// \brief Get the default similarity distance for grouping similar elements in the query analyzer
	static inline double defaultGroupSimilarityDistance()
	{
		return 0.60;
	}
	/// \brief Default prefix used for filenames containing the incremental changes of statistics
	static inline const char* defaultStatisticsFilePrefix()
	{
		return "stats_";
	}
	/// \brief Default extension used used for filenames containing the incremental changes of statistics
	static inline const char* defaultStatisticsFileExtension()
	{
		return ".bin";
	}
	///\brief Number of blocks used by default by a statistics map
	static inline int defaultStatisticsNofBlocks()
	{
		return 100000;
	}
	///\brief Size of a chunk used by default for statistics
	static inline int defaultStatisticsMsgChunkSize()
	{
		return 100000;
	}

	///\brief Size of a position info block that leads to flushing it to the database and opening a new one
	static inline int maxPosInfoBlockSize()
	{
		return 1024;
	}

	///\brief Size of a feature frequency block that leads to flushing it to the database and opening a new one
	static inline int maxFfBlockSize()
	{
		return 1024;
	}

	///\brief Size of a boolean block for representing sets that leads to flushing it to the database and opening a new one
	static inline int maxBooleanBlockSize()
	{
		return 1024;
	}

	///\brief Size of a block for representing structures that leads to flushing it to the database and opening a new one
	static inline int maxStructBlockSize()
	{
		return 1024;
	}

	///\brief Minimum OS file size
	///\note Used to approximate disk usage by counting sum of file sizes rounded to this value
	///\remark should be probed
	static inline int platformMinimumFileSize()
	{
		return 4096;
	}

	///\brief Ratio of the total size that decides wheter a block is replaced completely in a transaction (defines what fill grade leads to flushing it to the database and opening a new one)
	///\note Should be a value between 0.5 and 1.0
	///\note A value closer to 1.0 may potentially slow down query (not proven to be true), but probably be a little bit more efficient in insert and update as fewer blocks are written
	///\remark should maybe be made configurable in the future, but currently the default seems reasonable, probably an issue of over-configuration, measure first, we simply do not know enough
	static inline float minimumBlockFillRatio()
	{
		return 0.75;
	}

	///\brief Ratio of tolerated block size above the declared maximum for blocks that can be split (a very large document may occupy a larger block size because the document is the entity that cannot be split)
	///\note Should be a value greater or equal to 1.0
	///\note A value closer to 1.0 may potentially slow down query (not proven to be true), but insert and update will be more efficient as fewer blocks are written
	///\remark should maybe be made configurable in the future, but currently the default seems reasonable, probably a matter of over-configuration, measure first
	static inline float maximumBlockFillRatio()
	{
		return 1.4;
	}
};
}//namespace
#endif
