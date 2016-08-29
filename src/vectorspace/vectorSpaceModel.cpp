/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Implementation of the vector space model interface
#include "vectorSpaceModel.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/vectorSpaceModelInstanceInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <armadillo>

class VectorSpaceModelInstance
	:public VectorSpaceModelInstanceInterface
{
public:
	explicit VectorSpaceModelInstance( ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_){}

	virtual ~VectorSpaceModelInstance(){}

	virtual void initialize( std::size_t dim, std::size_t variations);

	virtual void load( const std::vector<double>& serialization);

	virtual std::vector<double> serialize() const;

	virtual std::vector<bool> getSimHash( const std::vector<double>& vec) const;

	virtual double calculateCosineSimilarity( const std::vector<double>& v1, const std::vector<double>& v2) const;

private:
	ErrorBufferInterface* m_errorhnd;
};


VectorSpaceModel::VectorSpaceModel( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_){}

VectorSpaceModelInstanceInterface* VectorSpaceModel::createModel() const
{
	try
	{
		return new VectorSpaceModelInstance( m_errorhnd);
	}
	CATCH_ERROR_MAP_RETURN( _TXT("error creating standard vector space model instance: %s"), *m_errorhnd, 0);
}


