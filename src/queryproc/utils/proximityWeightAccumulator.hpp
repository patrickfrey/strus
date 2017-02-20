/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef _STRUS_QUERYPROC_PROXIMITY_WEIGHT_ACCUMULATOR_HPP_INCLUDED
#define _STRUS_QUERYPROC_PROXIMITY_WEIGHT_ACCUMULATOR_HPP_INCLUDED
#include "positionWindow.hpp"
#include "private/internationalization.hpp"
#include "strus/base/string_format.hpp"
#include <cstring>

namespace strus {

/// \brief Forward declaration
class PostingIteratorInterface;

class ProximityWeightAccumulator
{
public:
	enum {MaxNofArguments=64};

	struct WeightArray
	{
		WeightArray()
			:arsize(0){}

		explicit WeightArray( std::size_t arsize_, double initvalue)
		{
			init( arsize_, initvalue);
		}
		WeightArray( double* ar_, std::size_t arsize_)
		{
			init( ar_, arsize_);
		}

		double ar[ MaxNofArguments];
		std::size_t arsize;

		void init( double* ar_, std::size_t arsize_)
		{
			arsize = arsize_;
			if (arsize_ > MaxNofArguments) throw strus::runtime_error( "number of features out of range");
			std::memcpy( ar, ar_, arsize_ * sizeof( *ar));
		}
		void init( std::size_t arsize_, double initvalue=0.0)
		{
			arsize = arsize_;
			if (arsize_ > MaxNofArguments) throw strus::runtime_error( "number of features out of range");
			for (std::size_t ai=0; ai<arsize; ++ai)
			{
				ar[ ai] = initvalue;
			}
		}

		void add( const WeightArray& o)
		{
			if (arsize != o.arsize) throw strus::runtime_error( "number of weights not the same (add)");
			for (std::size_t ai=0; ai<arsize; ++ai)
			{
				ar[ ai] += o.ar[ ai];
			}
		}
		void push( double value)
		{
			if (arsize > MaxNofArguments) throw strus::runtime_error( "number of features out of range");
			ar[ arsize++] = value;
		}

		double operator[]( std::size_t i) const
		{
			if (i >= arsize) throw strus::runtime_error( "array bound read");
			return ar[i];
		}
		double& operator[]( std::size_t i)
		{
			if (i >= arsize) throw strus::runtime_error( "array bound write");
			return ar[i];
		}

		double sum() const
		{
			double rt = 0.0;
			for (std::size_t ai=0; ai<arsize; ++ai) rt += ar[ai];
			return rt;
		}

		void multiply( const WeightArray& operand)
		{
			if (arsize != operand.arsize) throw strus::runtime_error( "number of weighting array elements do not match for multiplication");
			for (std::size_t ai=0; ai<arsize; ++ai) ar[ai] *= operand.ar[ai];
		}

		std::string tostring() const;
	};
	/// \brief Assigne weights to ar that sum up to 'sum'. each weight assigned consists of a part that is equal for all and a part that is in the proportions of base
	static void proportionalAssignment( WeightArray& ar, double sum, double constpart, const WeightArray& base);

	/// \brief Accumulate value for features in the same sentence
	static void weight_same_sentence(
		WeightArray& ar,
		double factor,
		const WeightArray& incrar,
		const std::size_t* window, std::size_t windowsize,
		const Index* maxdist_featar,
		PostingIteratorInterface** featar, std::size_t featarsize,
		PostingIteratorInterface** structar, std::size_t structarsize);

	/// \brief Accumulate value for subsequent values in the document that are immediately following in the query
	static void weight_imm_follow(
		WeightArray& ar,
		double factor,
		const WeightArray& incrar,
		const std::size_t* window, std::size_t windowsize,
		PostingIteratorInterface** featar, std::size_t featarsize);

	/// \brief Accumulate with inverse sqrt of distance
	static void weight_invdist(
		WeightArray& ar,
		double factor,
		const WeightArray& incrar,
		const std::size_t* window, std::size_t windowsize,
		PostingIteratorInterface** featar, std::size_t featarsize);

	/// \brief Accumulate with multiplication of a weight calculated from a table
	static void weight_invpos(
		WeightArray& ar,
		double factor,
		const WeightArray& incrar,
		const Index& firstpos,
		const std::size_t* window, std::size_t windowsize,
		PostingIteratorInterface** featar, std::size_t featarsize);

	static void weight_invpos(
		WeightArray& ar,
		double factor,
		const WeightArray& incrar,
		const Index& firstpos,
		PostingIteratorInterface** featar, std::size_t featarsize);
};

}//namespace
#endif


