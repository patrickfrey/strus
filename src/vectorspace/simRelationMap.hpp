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

/// \brief Structure for storing sample similarity relations
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
		unsigned int dim;
		arma::umat locations;
		arma::Col<unsigned short> values;

		ElementVector( const std::vector<Element>& vv, unsigned int dim_)
			:dim(dim_),locations( 2, 2*vv.size()), values( 2*vv.size())
		{
			std::vector<Element>::const_iterator vi = vv.begin(), ve = vv.end();
			for (std::size_t vidx=0; vi != ve; ++vi,vidx+=2)
			{
				locations( 0,vidx) = vi->coord_x;
				locations( 1,vidx) = vi->coord_y;
				values[ vidx] = vi->value;
				locations( 0,vidx+1) = vi->coord_y;
				locations( 1,vidx+1) = vi->coord_x;
				values[ vidx+1] = vi->value;
			}
		}
		arma::SpMat<unsigned short> matrix() const
		{
			return arma::SpMat<unsigned short>( locations, values, dim, dim);
		}
	};

public:
	explicit SimRelationMap( const std::vector<Element>& elements, unsigned int dim)
		:m_mat( ElementVector( elements, dim).matrix()){}
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

	std::string tostring() const;

private:
	arma::SpMat<unsigned short> m_mat;
};

}//namespace
#endif
