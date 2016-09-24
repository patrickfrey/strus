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
#include "simHash.hpp"
#include "lshModel.hpp"
#include "genModel.hpp"
#include "strus/base/fileio.hpp"
#include "strus/base/configParser.hpp"
#include <armadillo>
#include <memory>

using namespace strus;
#define MODULENAME "standard vector space model"

#define STRUS_LOWLEVEL_DEBUG

struct VectorSpaceModelConfig
{
	VectorSpaceModelConfig()
		:path(),dim(300),bits(64),variations(16)
		,simdist(160),mutations(10)
		,descendants(5),maxage(10),iterations(100){}
	VectorSpaceModelConfig( const std::string& config, ErrorBufferInterface* errorhnd)
		:path(),dim(300),bits(64),variations(32)
		,simdist(340),mutations(50)
		,descendants(10),maxage(20),iterations(20)
	{
		std::string src = config;
		if (extractStringFromConfigString( path, src, "path", errorhnd)){}
		if (extractUIntFromConfigString( dim, src, "dim", errorhnd)){}
		if (extractUIntFromConfigString( bits, src, "bit", errorhnd)){}
		if (extractUIntFromConfigString( variations, src, "var", errorhnd)){}
		if (extractUIntFromConfigString( simdist, src, "simdist", errorhnd)){}
		if (extractUIntFromConfigString( mutations, src, "mutations", errorhnd)){}
		if (extractUIntFromConfigString( descendants, src, "descendants", errorhnd)){}
		if (extractUIntFromConfigString( maxage, src, "maxage", errorhnd)){}
		if (extractUIntFromConfigString( iterations, src, "iterations", errorhnd)){}
		if (dim == 0 || bits == 0 || variations == 0 || mutations == 0 || descendants == 0 || maxage == 0 || iterations == 0)
		{
			strus::runtime_error(_TXT("error in vector space model configuration: dim, bits, var, mutations, descendants, maxage or iterations values must not be zero"));
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
	unsigned int simdist;
	unsigned int mutations;
	unsigned int descendants;
	unsigned int maxage;
	unsigned int iterations;
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
		:m_errorhnd(errorhnd_),m_config(config_,errorhnd_),m_lshmodel(0)
	{
		loadModelFromFile( m_config.path);
	}

	virtual ~VectorSpaceModelInstance()
	{
		if (m_lshmodel) delete( m_lshmodel);
	}

	virtual std::vector<Index> mapVectorToFeatures( const std::vector<double>& vec) const
	{
		try
		{
			std::vector<Index> rt;
			SimHash hash( m_lshmodel->simHash( arma::vec( vec)));
			std::vector<SimHash>::const_iterator ii = m_individuals.begin(), ie = m_individuals.end();
			for (std::size_t iidx=1; ii != ie; ++ii,++iidx)
			{
				if (ii->near( hash, m_config.simdist))
				{
					rt.push_back( iidx);
				}
			}
			return rt;
		}
		CATCH_ERROR_ARG1_MAP_RETURN( _TXT("error in instance of '%s' mapping vector to features: %s"), MODULENAME, *m_errorhnd, std::vector<Index>());
	}

	virtual unsigned int nofFeatures() const
	{
		return m_individuals.size();
	}

private:
	void loadModelFromFile( const std::string& path);

private:
	ErrorBufferInterface* m_errorhnd;
	VectorSpaceModelConfig m_config;
	LshModel* m_lshmodel;
	std::vector<SimHash> m_individuals;
};


void VectorSpaceModelInstance::loadModelFromFile( const std::string& path)
{
	if (path.empty()) throw strus::runtime_error(_TXT("no 'path' configuration variable defined, cannot load model"));
	std::string dump;
	unsigned int ec = readFile( path, dump);
	if (ec) throw strus::runtime_error(_TXT("failed to load model from file (errno %u): %s"), ec, ::strerror(ec));
	std::size_t itr = 0;
	std::auto_ptr<LshModel> lshmodel( LshModel::createFromSerialization( dump, itr));
	m_individuals = SimHash::createFromSerialization( dump, itr);
	m_lshmodel = lshmodel.release();

#ifdef STRUS_LOWLEVEL_DEBUG
	std::string txtfilename( path + ".in.txt");
	std::ostringstream txtdump;
	txtdump << "LSH:" << std::endl << m_lshmodel->tostring() << std::endl;
	txtdump << "GEN:" << std::endl;
	std::vector<SimHash>::const_iterator ii = m_individuals.begin(), ie = m_individuals.end();
	for (; ii != ie; ++ii)
	{
		txtdump << ii->tostring() << std::endl;
	}
	txtdump << std::endl;
	ec = writeFile( txtfilename, txtdump.str());
	if (ec)
	{
		throw strus::runtime_error(_TXT("failed to store debug text dump of instance loaded (system error %u: %s)"), ec, ::strerror(ec));
	}
#endif
}

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
			m_genmodel = new GenModel( m_config.simdist, m_config.mutations, m_config.descendants, m_config.maxage, m_config.iterations);
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
			m_samplear.push_back( m_lshmodel->simHash( arma::vec( vec)));
		}
		CATCH_ERROR_ARG1_MAP( _TXT("error adding sample vector to '%s' builder: %s"), MODULENAME, *m_errorhnd);
	}

	virtual void finalize()
	{
		try
		{
			m_resultar = m_genmodel->run( m_samplear);
		}
		CATCH_ERROR_ARG1_MAP( _TXT("error finalizing '%s' builder: %s"), MODULENAME, *m_errorhnd);
	}

	virtual bool store()
	{
		try
		{
			unsigned int ec;
#ifdef STRUS_LOWLEVEL_DEBUG
			std::string txtfilename( m_config.path + ".out.txt");
			std::ostringstream txtdump;
			std::vector<SimHash>::const_iterator ri = m_resultar.begin(), re = m_resultar.end();
			for (std::size_t ridx=0; ri != re; ++ri,++ridx)
			{
				txtdump << "[" << ridx << "] " << ri->tostring() << std::endl;
			}
			txtdump << "LSH:" << std::endl << m_lshmodel->tostring() << std::endl;
			txtdump << "GEN:" << std::endl << m_genmodel->tostring() << std::endl;
			ec = writeFile( txtfilename, txtdump.str());
			if (ec)
			{
				throw strus::runtime_error(_TXT("failed to store debug text dump of instance built (system error %u: %s)"), ec, ::strerror(ec));
			}
#endif
			if (m_config.path.empty()) throw strus::runtime_error(_TXT("failed to store built instance (no file configured)"));
			std::string dump;
			m_lshmodel->printSerialization( dump);
			SimHash::printSerialization( dump, m_resultar);
			ec = writeFile( m_config.path, dump);
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
	std::vector<SimHash> m_samplear;
	std::vector<SimHash> m_resultar;
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


