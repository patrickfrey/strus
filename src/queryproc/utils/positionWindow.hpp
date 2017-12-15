/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERYPROC_POSITION_WINDOW_HPP_INCLUDED
#define _STRUS_QUERYPROC_POSITION_WINDOW_HPP_INCLUDED
#include "strus/index.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "strus/base/stdint.h"
#include "strus/base/bitset.hpp"

namespace strus {

/// \brief Sliding window to iterate on the sets of posting iterator elements 
///		that are withing a defined proximity range:
class PositionWindow
{
public:
	enum EvaluationType {
		MinWin,		///< Minimal window containing at least a defined number of features [cardinality]
		MaxWin		///< Maximum window within a range (containing all features that satisfy the range condition, but at least a defined number of features [cardinality])
	};
	/// \brief We use fixed size arrays and restrict the maximum number of features to a reasonable amount.
	enum {MaxNofArguments=128};

	/// \brief Default constructor
	PositionWindow();
	/// \brief Constructor that fills the sliding window implemented as set 
	///		with the argument element start positions:
	PositionWindow( 
		PostingIteratorInterface** args,
		std::size_t nofargs,
		unsigned int range_,
		unsigned int cardinality_,
		Index firstpos_,
		EvaluationType evaluationType_);

	/// \brief Initializer method
	void init(
		PostingIteratorInterface** args,
		std::size_t nofargs,
		unsigned int range_,
		unsigned int cardinality_,
		Index firstpos_,
		EvaluationType evaluationType_);

	/// \brief Skip to the first window and return true, if there is one more:
	bool first();

	/// \brief Skip to the next window and return true, if there is one more:
	bool next();

	/// \brief Skip to the first window after pos and return true, if there is one more:
	bool skip( const Index& pos);

	/// \brief Return the number of elements of the current window (depends on evaluation type)
	unsigned int size() const
	{
		return m_windowsize;
	}

	/// \brief Return the span of the current window (distance from start to end, depends on evaluation type)
	unsigned int span() const
	{
		return (m_windowsize)?(m_posar[m_windowsize-1]-m_posar[0]+1):1;
	}

	/// \brief Return the starting position of the current window:
	unsigned int pos() const
	{
		return m_windowsize?m_posar[0]:0;
	}

	/// \brief Return a pointer to the elements of the current window
	const std::size_t* window() const
	{
		return m_windowsize?m_window:0;
	}

	/// \brief Get a bitset that specifies what elements in the current window are new (not part of a previous window visited)
	const strus::bitset<MaxNofArguments>& isnew_bitset() const
	{
		return m_isnew_bitset;
	}

private:
	/// \brief Get the size of the current minimal window:
	unsigned int getMinWinSize();
	/// \brief Get the size of the current maximal window within a proximity range:
	unsigned int getMaxWinSize();
	/// \brief Advance to the next candidate (after advancepos if defined):
	bool advance( const Index& advancepos=0);

private:
	PostingIteratorInterface* m_itrar[ MaxNofArguments];	///< element iterators
	std::size_t m_window[ MaxNofArguments];			///< window element references
	Index m_posar[ MaxNofArguments];			///< element positions
	unsigned int m_arsize;					///< current number of elements
	unsigned int m_range;					///< maximum proximity range
	unsigned int m_cardinality;				///< number of elements for a candidate window
	unsigned int m_windowsize;				///< size of current window in elements
	strus::bitset<MaxNofArguments> m_isnew_bitset;		///< bitset zu determine is an element in the window has not been visited yet
	EvaluationType m_evaluationType;			///< type of evaluation
};

}//namespace
#endif

