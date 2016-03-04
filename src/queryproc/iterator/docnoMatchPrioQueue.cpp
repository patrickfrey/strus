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
/// \brief Priority queue for joining document iterator docno matches
#include "docnoMatchPrioQueue.hpp"
#include "strus/errorBufferInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <cstring>

using namespace strus;

DocnoMatchPrioQueue::DocnoMatchPrioQueue( const std::vector<PostingIteratorReference>& args_, unsigned int cardinality_)
	:m_args(args_)
	,m_arsize(0)
	,m_quearsize(0)
	,m_cardinality((unsigned char)cardinality_)
	,m_curdocno(0)
	,m_curdocno_candidate(0)
	,m_maxdocno(0)
	,m_maxdocno_candidate(0)
{
	if (m_cardinality > MaxNofElements) throw strus::runtime_error(_TXT("cardinality out of range: %u"), m_cardinality);
	if (m_args.size() == 0) throw strus::runtime_error(_TXT("initializing allmatch priority queue with no arguments"));
	if (m_cardinality == 0) m_cardinality = m_args.size();
}

void DocnoMatchPrioQueue::init( const Index& docno_)
{
	// Empty the queue:
	m_arsize = 0;
	m_quearsize = 0;
	m_curdocno = 0;
	m_curdocno_candidate = 0;

	// Refill the queue:
	std::vector<PostingIteratorReference>::iterator ai = m_args.begin(), ae = m_args.end();
	for (; ai != ae; ++ai)
	{
		Index dn = (*ai)->skipDocCandidate( docno_);
		if (dn)
		{
			if (m_arsize >= MaxNofElements) throw strus::runtime_error( _TXT("number of arguments for get first matches iterator out of range: %u"), (unsigned int)m_arsize);
			insertElement( Element( dn, ai-m_args.begin()));
		}
	}
}

void DocnoMatchPrioQueue::insertElement( const Element& elem)
{
	m_ar[ m_arsize] = elem;
	unsigned char qi=0;
	for (; qi < m_quearsize && m_ar[ m_quear[ qi]].docno <= elem.docno; ++qi){}
	if (qi < m_quearsize)
	{
		std::memmove( m_quear+qi+1, m_quear+qi, (m_quearsize-qi)*sizeof(*m_quear));
	}
	m_quear[ qi] = m_arsize;
	++m_arsize;
	++m_quearsize;
}

void DocnoMatchPrioQueue::removeQueueElement( unsigned char qi)
{
	// reset the position of the deleted element:
	m_ar[ m_quear[ qi]].docno = 0;
	// remove queue element:
	if (qi+1 < m_quearsize)
	{
		std::memmove( m_quear+qi, m_quear+qi+1, (m_quearsize-qi-1)*sizeof(*m_quear));
	}
	--m_quearsize;
}

void DocnoMatchPrioQueue::shiftQueueElement( unsigned char qi, const Index& docno_)
{
	unsigned char aridx = m_quear[ qi];
	if (docno_ < m_ar[ aridx].docno)
	{
		throw strus::runtime_error(_TXT("bad all match prio queue operation (shift with smaller docno)"));
	}
	else
	{
		m_ar[ aridx].docno = docno_;

		for (++qi; qi < m_quearsize && m_ar[ m_quear[ qi]].docno < docno_; ++qi)
		{
			m_quear[ qi-1] = m_quear[ qi];
		}
		m_quear[ qi-1] = aridx;
	}
}

Index DocnoMatchPrioQueue::skipDocCandidate( const Index& docno_)
{
	// Case [A] The input document number is bigger or equal a
	//	previously failed skipDocCandidate call:
	if (m_maxdocno_candidate && m_maxdocno_candidate < docno_)
	{
		// ... the call fails:
		return 0;
	}
	// Case [B] The input document number is the current matching docno:
	if (m_curdocno_candidate && m_curdocno_candidate == docno_)
	{
		// ... we return the matching candidate docno:
		return m_curdocno_candidate;
	}
	m_curdocno_candidate = 0;

	// Case [C.1] The current queue does not contain a sufficient number of elements,
	// Case [C.2] The call is a skip back:
	if (m_cardinality > m_quearsize || docno_ < m_ar[ m_quear[0]].docno)
	{
		// ... we have to rebuild the queue:
		init( docno_);
		if (m_cardinality > m_quearsize)
		{
			if (!m_maxdocno_candidate || m_maxdocno_candidate < docno_)
			{
				m_maxdocno_candidate = docno_;
			}
			return 0;
		}
	}
	// [Main loop] We continue as long as we have valid queue of size cardinality and 
	//	did not find yet a result -> We skip the topmost element
	//	of the queue to the biggest element of the smallest docno set of size 
	//	cardinality (top elements of the ascending ordered priotity queue).
	//	If the function input docno is bigger, we change the target to the input docno.
	//	This is the next candidate to skip to in order to get to a configuration,
	//	with the top elements of the queue beeing equal and the least upper bound
	//	of the input docno:
	Index dn = m_ar[ m_quear[ m_cardinality-1]].docno;
	if (dn < docno_) dn = docno_;
	while (m_cardinality <= m_quearsize && m_ar[ m_quear[ 0]].docno < dn)
	{
		Index dn_next = m_args[ m_ar[ m_quear[ 0]].argidx]->skipDocCandidate( dn);
		if (dn_next == 0)
		{
			removeQueueElement( 0);
			if (m_cardinality > m_quearsize)
			{
				// ... with less than cardinality elements, we know that
				//	this request always fails:
				if (!m_maxdocno_candidate || m_maxdocno_candidate < docno_)
				{
					m_maxdocno = docno_?(docno_-1):0;
				}
			}
		}
		else
		{
			shiftQueueElement( 0, dn_next);
		}
		dn = m_ar[ m_quear[ m_cardinality-1]].docno;
		if (dn < docno_) dn = docno_;
	}
	if (m_cardinality > m_quearsize)
	{
		// ... with less than cardinality elements, we know that
		//	this request always fails:
		if (!m_maxdocno_candidate || m_maxdocno_candidate < docno_)
		{
			m_maxdocno = docno_?(docno_-1):0;
		}
		return 0;
	}
	// Return the found result and set the current document number to it:
	return m_curdocno_candidate=dn;
}

Index DocnoMatchPrioQueue::skipDoc( const Index& docno_)
{
	// Case [A] The input document number is bigger or equal a
	//	previously failed skipDoc call:
	if (m_maxdocno && m_maxdocno < docno_)
	{
		return 0;
	}
	// Case [B] The input document number is the current matching docno:
	if (m_curdocno && m_curdocno == docno_)
	{
		return m_curdocno;
	}
	m_curdocno = 0;

	// [Main loop]
	Index dn = docno_;
	for(;;)
	{
		// [1] Get the first candidate:
		dn = skipDocCandidate( dn);
		if (!dn)
		{
			// ... we did not even find a candidate, return null and adapt the
			// maximum document number that possibly can deliver a valid result:
			if (!m_maxdocno || m_maxdocno < docno_)
			{
				m_maxdocno = docno_?(docno_-1):0;
			}
			return 0;
		}
		// [2] Check if the candidate result matches by counting the skipDoc matches of the
		//	the topmost elements of the queue with same docno:
		unsigned char matchcnt = 0;
		unsigned char qi=0;
		for (; qi<m_quearsize && matchcnt<m_cardinality && dn == m_ar[ m_quear[ qi]].docno; qi++)
		{
			if (dn==m_args[ m_ar[ m_quear[ qi]].argidx]->skipDoc(dn))
			{
				++matchcnt; //... found a match
			}
		}
		if (matchcnt == m_cardinality)
		{
			// ... we've got a match, exit loop:
			m_curdocno = dn;
			break;
		}
		else
		{
			// ... we did not find a match in the current configuration,
			// so we have to try another one.
			dn += 1;
		}
	}
	return m_curdocno;
}

DocnoMatchPrioQueue::CandidateList DocnoMatchPrioQueue::getCandidateList()
{
	CandidateList rt;
	if (m_quearsize == 0) return rt;

	Index dn = m_ar[m_quear[0]].docno;
	rt.arsize = 1;
	rt.ar[0] = m_args[ m_ar[m_quear[0]].argidx].get();

	unsigned char qi=1,qe=m_quearsize;
	for (; qi<qe && m_ar[m_quear[qi]].docno == dn; ++qi)
	{
		rt.ar[qi] = m_args[ m_ar[m_quear[qi]].argidx].get();
	}
	return rt;
}

