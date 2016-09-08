/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Structure for storing similarity relations
#ifndef _STRUS_VECTOR_SPACE_MODEL_SIMILARITY_RELATION_MAP_HPP_INCLUDED
#define _STRUS_VECTOR_SPACE_MODEL_SIMILARITY_RELATION_MAP_HPP_INCLUDED
#include "simHash.hpp"
#include "strus/index.hpp"
#include <vector>
#include <map>
#include <armadillo>

namespace strus {

/// \brief Structure for storing similarity relations
class SimRelationMap
{
public:
	explicit SimRelationMap( std::size_t nofSamples=0)
		:m_mat( nofSamples, nofSamples){}
	SimRelationMap( const SimRelationMap& o)
		:m_mat(o.m_mat){}

	enum State
	{
		Free=0,
		Occupied=1
	};

	void defineRelation( std::size_t from, std::size_t to, unsigned short value)
	{
		m_mat( from, to) = value;
		m_mat( to, from) = value;
	}

	typedef arma::SpMat<unsigned short> Matrix;
	class Row
	{
	public:
		typedef Matrix::const_row_iterator const_iterator;

		Row( const const_iterator& begin_, const const_iterator& end_)
			:m_begin(begin_),m_end(end_){}

		const_iterator begin() const	{return m_begin;}
		const_iterator end() const	{return m_end;}

	private:
		const_iterator m_begin;
		const_iterator m_end;
	};

	Row row( unsigned int rowidx) const
	{
		return Row( m_mat.begin_row( rowidx), m_mat.end_row( rowidx));
	}

	std::vector<std::size_t> getHotspotList() const;

private:
	arma::SpMat<unsigned short> m_mat;
};

}//namespace
#endif
