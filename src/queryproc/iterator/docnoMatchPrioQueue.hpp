/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Priority queue for joining document iterator docno matches

#ifndef _STRUS_DOCNO_MATCH_PRIORITY_QUEUE_HPP_INCLUDED
#define _STRUS_DOCNO_MATCH_PRIORITY_QUEUE_HPP_INCLUDED
#include "strus/storage/index.hpp"
#include "strus/reference.hpp"
#include "strus/postingIteratorInterface.hpp"
#include <vector>

namespace strus
{

/// \brief Iterator on the all document matches
class DocnoMatchPrioQueue
{
public:
	typedef Reference< PostingIteratorInterface> PostingIteratorReference;

	/// \brief Constructor
	DocnoMatchPrioQueue(
		const std::vector<PostingIteratorReference>& args_,
		unsigned int cardinality_);
	/// \brief Destructor
	~DocnoMatchPrioQueue(){}

	/// \brief The maximum number of arguments is fixed, because we use a static array for the queue
	enum {MaxNofElements=256};

	/// \brief Element of the all match priority queue
	struct Element
	{
		Index docno;			///< element document number
		unsigned char argidx;		///< index into argument posting iterators passed with queue constructor

		Element()
			:docno(0),argidx(0){}
		Element( const Index& docno_, unsigned char argidx_)
			:docno(docno_),argidx(argidx_){}
		Element( const Element& o)
			:docno(o.docno),argidx(o.argidx){}
	};

	/// \brief List of top elements with same docno of the queue
	struct CandidateList
	{
		PostingIteratorInterface* ar[ MaxNofElements];	///< top element iterators
		std::size_t arsize;				///< number of top element iterators
	};

	/// \brief Find the first match queue configuration with the first [cardinality] elements 
	///	having the same docno that is the least upper bound of the document number passed.
	/// \note The cardinality of the queue is used to calculate the next document skip
	/// \param[in] docno_ the minimal document number
	/// \return the top element document number of the found candidate queue configuration
	Index skipDoc( const Index& docno_);

	/// \brief Find the first candidate queue configuration with the first [cardinality] elements 
	///	having the same docno that is the least upper bound of the document number passed.
	/// \note The cardinality of the queue is used to calculate the next document candidate
	/// \param[in] docno_ the minimal document number
	/// \return the top element document number of the found candidate queue configuration
	Index skipDocCandidate( const Index& docno_);

	/// \brief Get the current top elements with same docno of the queue
	/// \return the list of top queue elements with same docno
	CandidateList getCandidateList();

	/// \brief Get the argument iterators in their order passed to this
	const std::vector<PostingIteratorReference>& args() const
	{
		return m_args;
	}

private:
	/// \brief Initialize the queue, so that elements are ordered ascending by document number and all document numbers are bigger or equal the passed document number
	///	Fill the queue with candidates, not with real matches
	/// \param[in] docno_ the minimal document number
	void init( const Index& docno_);

	/// \brief Insert an element into the queue preserving the ascending order of element document numbers
	/// \param[in] elem to insert
	void insertElement( const Element& elem);

	/// \brief Remove the element addressed  by queue index from the queue.
	/// \param[in] qi queue element index to remove
	void removeQueueElement( unsigned char qi);

	/// \brief Shift up the queue element addressed by queue index to a place specified by document number
	/// \param[in] qi element index of the queue element to shift up
	/// \param[in] docno_ new document number (must be bigger or equal than before) of the element to shift
	void shiftQueueElement( unsigned char qi, const Index& docno_);

private:
	std::vector<PostingIteratorReference> m_args;	///< argument posting iterators
	Element m_ar[ MaxNofElements];			///< processed elements
	unsigned char m_quear[ MaxNofElements];		///< number of processed elements
	unsigned char m_arsize;				///< number of currently processed elements
	unsigned char m_quearsize;			///< size of queue
	unsigned char m_cardinality;			///< cardinality of the result set
	Index m_curdocno;				///< current last docno match
	Index m_curdocno_candidate;			///< current last docno match candidate
	Index m_maxdocno;				///< first document found without upperbound match
	Index m_maxdocno_candidate;			///< first document found without upperbound match candidate
};

}//namespace
#endif

