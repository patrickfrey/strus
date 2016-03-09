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
	const Index* maxdist_featar,
	PostingIteratorInterface** featar, std::size_t featarsize,
	PostingIteratorInterface** structar, std::size_t structarsize)
{
	Index win_pos[ MaxNofArguments];
	Index smark_pos[ MaxNofArguments];

	std::size_t wi = 0;
	for (; wi < windowsize; ++wi)
	{
		win_pos[ wi] = featar[ window[ wi]]->posno();
		smark_pos[ wi] = 0;
		std::size_t si = 0;
		for (; si < structarsize; ++si)
		{
			Index smark = structar[si]->skipPos( win_pos[ wi]+1);
			if (smark && (!smark_pos[ wi] || smark < smark_pos[ wi]))
			{
				smark_pos[ wi] = smark;
			}
		}
	}
	for (wi=0; wi < windowsize; ++wi)
	{
		std::size_t pi=wi+1;
		for (; pi < windowsize; ++pi)
		{
			if (smark_pos[ wi] && smark_pos[ wi] <= win_pos[ pi]) continue;
			//... count same sentence only if there is no end of sentence marker inbetween
			if (win_pos[ wi] + maxdist_featar[ window[ wi]] < win_pos[ pi]) continue;
			//... count same sentence only withing a maximum proximity range

			ar[ window[ pi]] += incrar[ window[ wi]] / (1.0 - incrar[ window[ pi]]) * factor;
			ar[ window[ wi]] += incrar[ window[ pi]] / (1.0 - incrar[ window[ wi]]) * factor;
		}
	}
}

void ProximityWeightAccumulator::weight_imm_follow(
	WeightArray& ar,
	double factor,
	const WeightArray& incrar,
	const std::size_t* window, std::size_t windowsize,
	PostingIteratorInterface** featar, std::size_t featarsize)
{
	std::size_t wi=1;
	for (; wi < windowsize; ++wi)
	{
		if (window[wi] == window[wi-1]+1
		&& featar[window[wi]]->posno() == featar[window[wi-1]]->posno()+1)
		{
			ar[ window[ wi]] += incrar[ window[ wi-1]] / (1.0 - incrar[ window[ wi]]) * factor;
			ar[ window[ wi-1]] += incrar[ window[ wi]] / (1.0 - incrar[ window[ wi-1]]) * factor;
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
		win_pos[ wi] = featar[ window[ wi]]->posno();
	}
	for (wi=0; wi < windowsize; ++wi)
	{
		std::size_t pi=wi+1;
		for (; pi < windowsize; ++pi)
		{
			double dist = win_pos[ pi] - win_pos[ wi];
			double weight = 1.0 / sqrt( dist+1);
			ar[ window[ pi]] += incrar[ window[ wi]] / (1.0 - incrar[ window[ pi]]) * weight * factor;
			ar[ window[ wi]] += incrar[ window[ pi]] / (1.0 - incrar[ window[ wi]]) * weight * factor;
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
		unsigned int pos = featar[ fi]->posno();
		if (pos)
		{
			double weight = 1.0 / sqrt( pos-firstpos+1);
			ar[ fi] += incrar[ fi] * weight * factor;
		}
	}
}


