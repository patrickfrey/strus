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
#include "simhash.hpp"
#include "lshmodel.hpp"
#include "genmodel.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/configParser.hpp"
#include <armadillo>

using namespace strus;
#define MODULENAME "standard vector space model"

struct VectorSpaceModelConfig
{
	VectorSpaceModelConfig()
		:path(),dim(300),bits(64),variations(16),threshold_sim(0.9)
		,threshold_simdist(160),threshold_nbdist(260),mutations(10)
		,descendants(5),maxage(10),chunksize(1000){}
	VectorSpaceModelConfig( const std::string& config, ErrorBufferInterface* errorhnd)
		:path(),dim(300),bits(64),variations(16),threshold_sim(0.9)
		,threshold_simdist(160),threshold_nbdist(260),mutations(10)
		,descendants(5),maxage(10),chunksize(1000)
	{
		std::string src = config;
		if (extractStringFromConfigString( path, src, "path", errorhnd)){}
		if (extractUIntFromConfigString( dim, src, "dim", errorhnd)){}
		if (extractUIntFromConfigString( bits, src, "bit", errorhnd)){}
		if (extractUIntFromConfigString( variations, src, "var", errorhnd)){}
		if (extractFloatFromConfigString( threshold_sim, src, "thsim", errorhnd)){}
		if (extractUIntFromConfigString( threshold_simdist, src, "simdist", errorhnd)){}
		if (extractUIntFromConfigString( threshold_nbdist, src, "nbdist", errorhnd)){}
		if (extractUIntFromConfigString( mutations, src, "mutations", errorhnd)){}
		if (extractUIntFromConfigString( descendants, src, "descendants", errorhnd)){}
		if (extractUIntFromConfigString( maxage, src, "maxage", errorhnd)){}
		if (extractUIntFromConfigString( chunksize, src, "chunksize", errorhnd)){}
		if (dim == 0 || bits == 0 || variations == 0 || mutations == 0 || descendants == 0 || maxage == 0)
		{
			strus::runtime_error(_TXT("error in vector space model configuration: dim, bits, var, mutations, descendants or maxage values must not be zero"));
		}
		if (chunksize == 0)
		{
			chunksize = 1;
		}
		if (errorhnd->hasError())
		{
			strus::runtime_error(_TXT("error loading vector space model configuration: %s"), errorhnd->fetchError());
		}
	}

	std::string path;
	unsigned int dim;
	unsigned int bits;
	unsigned int variations;
	double threshold_sim;
	unsigned int threshold_simdist;
	unsigned int threshold_nbdist;
	unsigned int mutations;
	unsigned int descendants;
	unsigned int maxage;
	unsigned int chunksize;
};

struct VectorSpaceModelData
{
	VectorSpaceModelData( const VectorSpaceModelConfig& config_)
		:config(config_){}

	VectorSpaceModelConfig config;
};

class VectorSpaceModelInstance
	:public VectorSpaceModelInstanceInterface
{
public:
	VectorSpaceModelInstance( const std::string& config_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_config(config_,errorhnd_)
	{}

	virtual ~VectorSpaceModelInstance(){}

	virtual std::vector<Index> mapVectorToFeatures( const std::vector<double>& vec) const
	{
		try
		{
			std::vector<Index> rt;
			return rt;
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in instance of '%s' mapping vector to features: %s"), MODULENAME, *m_errorhnd, std::vector<Index>());
	}

private:
	ErrorBufferInterface* m_errorhnd;
	VectorSpaceModelConfig m_config;
};


class VectorSpaceModelBuilder
	:public VectorSpaceModelBuilderInterface
{
public:
	VectorSpaceModelBuilder( const std::string& config_, ErrorBufferInterface* errorhnd_)
		:m_errorhnd(errorhnd_),m_config(config_,errorhnd_),m_lshmodel(0),m_genmodel(0)
	{
		try
		{
			m_lshmodel = new LshModel( m_config.dim, m_config.bits, m_config.variations);
			m_genmodel = new GenModel( m_config.threshold_simdist, m_config.threshold_nbdist, m_config.mutations, m_config.descendants, m_config.maxage);
		}
		catch (const std::exception& err)
		{
			if (m_lshmodel) delete m_lshmodel;
			if (m_genmodel) delete m_genmodel;
			m_lshmodel = 0;
			m_genmodel = 0;
			throw strus::runtime_error( _TXT("failed to build %s: %s"), MODULENAME, err.what());
		}
	}
	virtual ~VectorSpaceModelBuilder()
	{
		if (m_lshmodel) delete m_lshmodel;
		if (m_genmodel) delete m_genmodel;
	}

	virtual void addSampleVector( const std::vector<double>& vec)
	{
		try
		{
			m_samplear.push_back( arma::vec( vec));
			m_genmodel->addSample( m_lshmodel->simHash( vec));
			if (m_samplear.size() % m_config.chunksize == 0)
			{
				m_genmodel->iteration();
				if (m_samplear.size() % (m_config.chunksize*10) == 0)
				{
					m_genmodel->unification();
				}
			}
		}
		CATCH_ERROR_ARG1_MAP( _TXT("error adding sample vector to '%s' builder: %s"), MODULENAME, *m_errorhnd);
	}

	virtual void finalize()
	{
		try
		{
			m_genmodel->iteration();
			m_genmodel->unification();
		}
		CATCH_ERROR_ARG1_MAP( _TXT("error finalizing '%s' builder: %s"), MODULENAME, *m_errorhnd);
	}

	virtual bool store()
	{
		try
		{
			if (m_config.path.empty()) throw strus::runtime_error(_TXT("failed to store built instance (no file configured)"));
			std::string dump( m_lshmodel->serialize());
			unsigned int ec = writeFile( m_config.path, dump);
			if (ec)
			{
				throw strus::runtime_error(_TXT("failed to store built instance (system error %u: %s)"), ec, ::strerror(ec));
			}
			return true;
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error storing '%s' builder: %s"), MODULENAME, *m_errorhnd, false);
	}

private:
	ErrorBufferInterface* m_errorhnd;
	VectorSpaceModelConfig m_config;
	LshModel* m_lshmodel;
	GenModel* m_genmodel;
	std::vector<arma::vec> m_samplear;
};


VectorSpaceModel::VectorSpaceModel( ErrorBufferInterface* errorhnd_)
	:m_errorhnd(errorhnd_){}


VectorSpaceModelInstanceInterface* VectorSpaceModel::createInstance( const std::string& config) const
{
	try
	{
		return new VectorSpaceModelInstance( config, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' instance: %s"), MODULENAME, *m_errorhnd, 0);
}

VectorSpaceModelBuilderInterface* VectorSpaceModel::createBuilder( const std::string& config) const
{
	try
	{
		return new VectorSpaceModelBuilder( config, m_errorhnd);
	}
	CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error creating '%s' builder: %s"), MODULENAME, *m_errorhnd, 0);
}


