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
	struct Element
	{
		unsigned int coord_x;
		unsigned int coord_y;
		unsigned short value;

		Element( unsigned int x, unsigned int y, unsigned short v)
			:coord_x(x),coord_y(y),value(v){}
		Element( const Element& o)
			:coord_x(o.coord_x),coord_y(o.coord_y),value(o.value){}
	};

private:
	struct ElementVector
	{
		arma::umat locations;
		arma::Col<unsigned short> values;

		ElementVector( const std::vector<Element>& vv)
		{
			std::vector<Element>::const_iterator vi = vv.begin(), ve = vv.end();
			for (; vi != ve; ++vi)
			{
				locations << vi->coord_x << vi->coord_y << arma::endr;
				values << vi->value;
			}
		}
		arma::SpMat<unsigned short> matrix() const
		{
			return arma::SpMat<unsigned short>( locations, values);
		}
	};

public:
	explicit SimRelationMap( const std::vector<Element>& elements)
		:m_mat( ElementVector( elements).matrix()){}
	SimRelationMap( const SimRelationMap& o)
		:m_mat(o.m_mat){}

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

private:
	arma::SpMat<unsigned short> m_mat;
};

}//namespace
#endif
