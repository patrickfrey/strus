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
#include "strus/vectorSpaceModelBuilderInterface.hpp"
#include "private/internationalization.hpp"
#include "private/errorUtils.hpp"
#include <armadillo>

using namespace strus;

struct VectorSpaceModelConfig
{
	VectorSpaceModelConfig()
		:path(),dim(300),bits(64),variants(16),threshold_sim(0.9),threshold_dist(160){}
	VectorSpaceModelConfig( const std::string& config, ErrorBufferInterface* errorhnd)
		:path(),dim(300),bits(64),variants(16),threshold_sim(0.9),threshold_dist(160)
	{
		std::string src = config;
		if (extractStringFromConfigString( path, src, "path", errorhnd)){}
		if (extractUIntFromConfigString( dim, src, "dim", errorhnd)){}
		if (extractUIntFromConfigString( bits, src, "bits", errorhnd)){}
		if (extractUIntFromConfigString( variants, src, "variants", errorhnd)){}
		if (extractFloatFromConfigString( threshold_sim, src, "thsim", errorhnd)){}
		if (extractUIntFromConfigString( threshold_dist, src, "thdist", errorhnd)){}
		if (dim == 0 || bits == 0 || variants == 0)
		{
			strus::runtime_error(_TXT("error in vector space model configuration: dim, bits or variants value must not be zero"));
		}
		if (errorhnd->hasError())
		{
			strus::runtime_error(_TXT("error loading vector space model configuration: %s"), errorhnd->fetchError());
		}
	}

	std::string path;
	unsigned int dim;
	unsigned int bits;
	unsigned int variants;
	double threshold_sim;
	unsigned int threshold_dist;
};


class VectorSpaceModelInstance
	:public VectorSpaceModelInstanceInterface
{
public:
	VectorSpaceModelInstance( const std::string& config_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_config(config_,errorhnd_)
	{}

	virtual ~VectorSpaceModelInstance(){}

	virtual std::vector<Index> mapVectorToFeatures( const std::vector<double>& vec) const;

private:
	ErrorBufferInterface* m_errorhnd;
	VectorSpaceModelConfig m_config;
};


class VectorSpaceModelBuilder
	:public VectorSpaceModelBuilderInterface
{
public:
	VectorSpaceModelBuilder( const std::string& config_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_config(config_,errorhnd_)
	{}
	virtual ~VectorSpaceModelBuilder(){}

	virtual void addSampleVector( const std::vector<double>& vec);

	virtual void finalize();

	virtual bool store();

private:
	ErrorBufferInterface* m_errorhnd;
	VectorSpaceModelConfig m_config;
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


