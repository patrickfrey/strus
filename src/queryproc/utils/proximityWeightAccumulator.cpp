/*
 * Copyright (c) 2014 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "proximityWeightAccumulator.hpp"
#include "strus/postingIteratorInterface.hpp"
#include "private/internationalization.hpp"
#include <limits>
#include <cmath>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace strus;

std::string ProximityWeightAccumulator::WeightArray::tostring() const
{
	std::ostringstream rt;
	for (std::size_t ai=0; ai < arsize; ++ai)
	{
		if (ai) rt << ' ';
		rt << std::setprecision(7) << std::fixed << ar[ai];
	}
	return rt.str();
}

void ProximityWeightAccumulator::proportionalAssignment( WeightArray& ar, double sum, double constpart, const WeightArray& base)
{
	double totsum = base.sum();
	if (ar.arsize != base.arsize || sum < constpart || totsum < std::numeric_limits<double>::epsilon())
	{
		throw strus::runtime_error(_TXT("invalid parameters in call to proportional assignment"));
	}
	for (std::size_t ai=0; ai < ar.arsize; ++ai)
	{
		double part = base.ar[ ai] / totsum;
		ar.ar[ ai] += (part * (sum - constpart)) + (constpart/ar.arsize);
	}
}

void ProximityWeightAccumulator::weight_same_sentence(
	WeightArray& ar,
	double factor,
	const WeightArray& incrar,
	const std::size_t* window, std::size_t windowsize,
	PostingIteratorInterface** featar, std::size_t featarsize,
	const std::pair<Index,Index>& structframe)
{
	std::size_t wi = 0;
	double weight = 0.0;
	for (; wi < windowsize; ++wi)
	{
		if (!structframe.second || featar[ window[ wi]]->posno() < structframe.second)
		{
			weight += incrar[ window[wi]];
		}
		else
		{
			break;
		}
	}
	for (wi=0; wi < windowsize; ++wi)
	{
		if (!structframe.second || featar[ window[ wi]]->posno() < structframe.second)
		{
			ar[ window[wi]] += incrar[ window[wi]] * (weight - incrar[ window[wi]]) * factor;
		}
		else
		{
			break;
		}
	}
}

void ProximityWeightAccumulator::weight_invdist(
	WeightArray& ar,
	double factor,
	const WeightArray& incrar,
	const std::size_t* window, std::size_t windowsize,
	PostingIteratorInterface** featar, std::size_t featarsize)
{
	std::size_t wi = 0;
	Index win_pos[ MaxNofArguments];

	for (wi = 0; wi < windowsize; ++wi)
	{
		win_pos[ wi] = featar[ window[ wi]] ? featar[ window[ wi]]->posno() : 0;
	}
	for (wi=0; wi < windowsize; ++wi)
	{
		std::size_t pi=wi+1;
		for (; pi < windowsize; ++pi)
		{
			if (!win_pos[ pi]) continue;
			double dist = win_pos[ pi] - win_pos[ wi];
			double weight = 1.0 / sqrt( dist+1);
			double order_weight = window[ pi] > window[ wi] ? (1.0 + 0.5 / (window[ pi] - window[ wi])) : 1.0;
			ar[ window[ pi]] += incrar[ window[ wi]] / (1.0 - incrar[ window[ pi]]) * weight * factor * order_weight;
			ar[ window[ wi]] += incrar[ window[ pi]] / (1.0 - incrar[ window[ wi]]) * weight * factor * order_weight;
		}
	}
}

void ProximityWeightAccumulator::weight_invpos(
	WeightArray& ar,
	double factor,
	const WeightArray& incrar,
	const Index& firstpos,
	const std::size_t* window, std::size_t windowsize,
	PostingIteratorInterface** featar, std::size_t featarsize)
{
	std::size_t wi=0;
	for (; wi < windowsize; ++wi)
	{
		if (!featar[ window[wi]]) continue;
		unsigned int pos = featar[ window[wi]]->posno();
		if (pos)
		{
			double weight = 1.0 / sqrt( pos-firstpos+1);
			ar[ window[wi]] += incrar[ window[wi]] * weight * factor;
		}
	}
}

void ProximityWeightAccumulator::weight_invpos(
	WeightArray& ar,
	double factor,
	const WeightArray& incrar,
	const Index& firstpos,
	PostingIteratorInterface** featar, std::size_t featarsize)
{
	std::size_t fi=0;
	for (; fi < featarsize; ++fi)
	{
		if (!featar[ fi]) continue;
		unsigned int pos = featar[ fi]->posno();
		if (pos)
		{
			double weight = 1.0 / sqrt( pos-firstpos+1);
			ar[ fi] += incrar[ fi] * weight * factor;
		}
	}
}


