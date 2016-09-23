/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Test program
#include "strus/lib/vectorspace_std.hpp"
#include "strus/lib/error.hpp"
#include "strus/index.hpp"
#include "strus/vectorSpaceModelInterface.hpp"
#include "strus/vectorSpaceModelInstanceInterface.hpp"
#include "strus/vectorSpaceModelBuilderInterface.hpp"
#include "strus/errorBufferInterface.hpp"
#include "strus/base/configParser.hpp"
#include <armadillo>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <memory>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include <limits>

#undef STRUS_LOWLEVEL_DEBUG

static void initRandomNumberGenerator()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	unsigned int seed = (now->tm_year+10000) + (now->tm_mon+100) + (now->tm_mday+1);
	std::srand( seed+2);
}

static std::vector<double> convertVectorStd( const arma::vec& vec)
{
	std::vector<double> rt;
	arma::vec::const_iterator vi = vec.begin(), ve = vec.end();
	for (; vi != ve; ++vi)
	{
		rt.push_back( *vi);
	}
	return rt;
}

static std::vector<double> createSimilarVector( const std::vector<double>& vec_, double maxCosSim)
{
	arma::vec vec( vec_);
	arma::vec orig( vec);
	for (;;)
	{
		unsigned int idx = rand() % vec.size();
		double elem = vec[ idx];
		if ((rand() & 1) == 0)
		{
			elem -= elem / 10;
			if (elem < 0.0) continue;
		}
		else
		{
			elem += elem / 10;
		}
		double oldelem = vec[ idx];
		vec[ idx] = elem;
		double cosSim = arma::norm_dot( vec, orig);
		if (maxCosSim > cosSim)
		{
			vec[ idx] = oldelem;
			break;
		}
	}
	return convertVectorStd( vec);
}

std::vector<double> createRandomVector( unsigned int dim)
{
	return convertVectorStd( arma::randu<arma::vec>( dim));
}

static strus::ErrorBufferInterface* g_errorhnd = 0;

int main( int argc, const char** argv)
{
	try
	{
		g_errorhnd = strus::createErrorBuffer_standard( 0, 0);
		if (!g_errorhnd) throw std::runtime_error("failed to create error buffer structure");

		initRandomNumberGenerator();
		std::string config( "path=test.vm;dim=300;bit=64;var=32;simdist=340;mutations=50;descendants=10;maxage=20;iterations=20");
		unsigned int nof_samples = 10;
		unsigned int dim = 300;

		if (argc > 3)
		{
			std::cerr << "Usage: " << argv[0] << " [<config>] [<number of sample vectors>]" << std::endl;
			throw std::runtime_error( "too many arguments (maximum 1 expected)");
		}
		if (argc >= 2)
		{
			config.append( argv[1]);
		}
		if (argc >= 3)
		{
			bool error = false;
			if (argv[2][0] >= '0' && argv[2][0] <= '9')
			{
				nof_samples = std::atoi( argv[2]);
			}
			else
			{
				error = true;
			}
			if (error) throw std::runtime_error( "number (non negative integer) expected as 2nd argument");
		}
		std::cerr << "model config: " << config << std::endl;
		std::string configsrc;
		(void)extractUIntFromConfigString( dim, configsrc, "dim", g_errorhnd);

		std::auto_ptr<strus::VectorSpaceModelInterface> vmodel( createVectorSpaceModel_std( g_errorhnd));
		if (!vmodel.get()) throw std::runtime_error("failed to create vector space model structure");
		std::auto_ptr<strus::VectorSpaceModelBuilderInterface> builder( vmodel->createBuilder( config));
		if (!builder.get()) throw std::runtime_error("failed to create vector space model builder structure");

		std::cerr << "create " << nof_samples << " sample vectors" << std::endl;
		std::vector<std::vector<double> > samplear;
		std::size_t sidx = 0;
		for (; sidx != nof_samples; ++sidx)
		{
			std::vector<double> vec;
			if (!sidx || rand() % 3 < 2)
			{
				vec = createRandomVector( dim);
			}
			else
			{
				std::size_t idx = rand() % sidx;
				vec = createSimilarVector( samplear[ idx], 0.90 + (rand() % 100) * 0.001);
			}
			samplear.push_back( vec);
			builder->addSampleVector( vec);
		}
		std::cerr << "building model" << std::endl;
		builder->finalize();
		if (g_errorhnd->hasError())
		{
			throw std::runtime_error( "building of model failed");
		}
		std::cerr << "store model" << std::endl;
		if (!builder->store())
		{
			throw std::runtime_error( "storing of model failed");
		}

		std::cerr << "load model to caegorize vectors" << std::endl;
		std::auto_ptr<strus::VectorSpaceModelInstanceInterface> categorizer( vmodel->createInstance( config));
		if (!categorizer.get())
		{
			throw std::runtime_error( "failed to load model built");
		}
		std::cerr << "loaded trained model with " << categorizer->nofFeatures() << " features" << std::endl;
		std::vector<std::vector<double> >::const_iterator si = samplear.begin(), se = samplear.end();
		for (sidx=0; si != se; ++si,++sidx)
		{
			std::vector<strus::Index> ctgar( categorizer->mapVectorToFeatures( *si));
			std::vector<strus::Index>::const_iterator ci = ctgar.begin(), ce = ctgar.end();
			std::cout << "[" << sidx << "] => ";
			for (std::size_t cidx=0; ci != ce; ++ci,++cidx)
			{
				if (cidx) std::cout << ", ";
				std::cout << *ci;
			}
			std::cout << std::endl;
		}
		std::cerr << "done" << std::endl;
		return 0;
	}
	catch (const std::runtime_error& err)
	{
		std::string msg;
		if (g_errorhnd && g_errorhnd->hasError())
		{
			msg.append( " (");
			msg.append( g_errorhnd->fetchError());
			msg.append( ")");
		}
		std::cerr << "error: " << err.what() << msg << std::endl;
		return -1;
	}
	catch (const std::bad_alloc& )
	{
		std::cerr << "out of memory" << std::endl;
		return -2;
	}
	catch (const std::logic_error& err)
	{
		std::string msg;
		if (g_errorhnd && g_errorhnd->hasError())
		{
			msg.append( " (");
			msg.append( g_errorhnd->fetchError());
			msg.append( ")");
		}
		std::cerr << "error: " << err.what() << msg << std::endl;
		return -3;
	}
}

