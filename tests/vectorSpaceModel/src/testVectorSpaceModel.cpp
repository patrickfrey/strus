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
#include "strus/base/stdint.h"
#include "sparseDim2Field.hpp"
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

#define STRUS_LOWLEVEL_DEBUG

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
		std::string config( "path=test.vm;dim=300;bit=64;var=32;simdist=340;eqdist=60;mutations=50;votes=13;descendants=10;maxage=20;iterations=50");
		unsigned int nofSamples = 1000;
		unsigned int dim = 0;

		if (argc > 3)
		{
			std::cerr << "Usage: " << argv[0] << " [<config>] [<number of sample vectors>]" << std::endl;
			throw std::runtime_error( "too many arguments (maximum 1 expected)");
		}
		if (argc >= 2)
		{
			config = argv[1];
		}
		if (argc >= 3)
		{
			bool error = false;
			if (argv[2][0] >= '0' && argv[2][0] <= '9')
			{
				nofSamples = std::atoi( argv[2]);
			}
			else
			{
				error = true;
			}
			if (error) throw std::runtime_error( "number (non negative integer) expected as 2nd argument");
		}
		std::cerr << "model config: " << config << std::endl;
		std::string configsrc = config;
		(void)extractUIntFromConfigString( dim, configsrc, "dim", g_errorhnd);

		std::auto_ptr<strus::VectorSpaceModelInterface> vmodel( createVectorSpaceModel_std( g_errorhnd));
		if (!vmodel.get()) throw std::runtime_error( g_errorhnd->fetchError());
		std::auto_ptr<strus::VectorSpaceModelBuilderInterface> builder( vmodel->createBuilder( config));
		if (!builder.get()) throw std::runtime_error( g_errorhnd->fetchError());

		std::cerr << "create " << nofSamples << " sample vectors" << std::endl;
		std::vector<std::vector<double> > samplear;
		std::size_t sidx = 0;
		for (; sidx != nofSamples; ++sidx)
		{
			std::vector<double> vec;
			if (!sidx || rand() % 3 < 2)
			{
				vec = createRandomVector( dim);
			}
			else
			{
				std::size_t idx = rand() % sidx;
				double sim = 0.90 + (rand() % 100) * 0.001;
				vec = createSimilarVector( samplear[ idx], sim);
			}
			samplear.push_back( vec);
			builder->addSampleVector( vec);
		}
		std::cerr << "create similarity matrix" << std::endl;
		unsigned int nofSimilarities = 0;
		strus::SparseDim2Field<unsigned char> expSimMatrix;
		strus::SparseDim2Field<double> simMatrix;
		{
			std::vector<arma::vec> samplevecar;
			for (sidx=0; sidx != nofSamples; ++sidx)
			{
				samplevecar.push_back( arma::vec( samplear[ sidx]));

				for (std::size_t oidx=0; oidx != sidx; ++oidx)
				{
					double sim = arma::norm_dot( samplevecar.back(), samplevecar[ oidx]);
					if (sim > 0.9)
					{
						nofSimilarities += 1;
						std::cerr << "sample " << sidx << " has " << sim << " similarity to " << oidx << std::endl;
						expSimMatrix( sidx, oidx) = 1;
						expSimMatrix( oidx, sidx) = 1;
					}
					if (sim > 0.8)
					{
						simMatrix( sidx, oidx) = sim;
						simMatrix( oidx, sidx) = sim;
					}
				}
			}
		}
		std::cerr << "building model" << std::endl;
		builder->finalize();
		if (g_errorhnd->hasError())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		std::cerr << "store model" << std::endl;
		if (!builder->store())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}

		std::cerr << "load model to categorize vectors" << std::endl;
		std::auto_ptr<strus::VectorSpaceModelInstanceInterface> categorizer( vmodel->createInstance( config));
		if (!categorizer.get())
		{
			throw std::runtime_error( g_errorhnd->fetchError());
		}
		std::cerr << "loaded trained model with " << categorizer->nofFeatures() << " features" << std::endl;
		typedef strus::SparseDim2Field<unsigned char> FeatureMatrix;
		FeatureMatrix featureMatrix;
		FeatureMatrix featureInvMatrix;
		std::vector<std::vector<double> >::const_iterator si = samplear.begin(), se = samplear.end();
		for (sidx=0; si != se; ++si,++sidx)
		{
			std::vector<strus::Index> ctgar( categorizer->mapVectorToFeatures( *si));
			std::vector<strus::Index>::const_iterator ci = ctgar.begin(), ce = ctgar.end();
			std::cout << "[" << sidx << "] => ";
			for (std::size_t cidx=0; ci != ce; ++ci,++cidx)
			{
				featureMatrix( sidx, *ci-1) = 1;
				featureInvMatrix( *ci-1, sidx) = 1;
				if (cidx) std::cout << ", ";
				std::cout << *ci;
			}
			std::cout << std::endl;
		}
		std::cerr << "build sample to sample feature relation matrix:" << std::endl;
		strus::SparseDim2Field<unsigned char> outSimMatrix;
		si = samplear.begin(), se = samplear.end();
		for (sidx=0; si != se; ++si,++sidx)
		{
			FeatureMatrix::const_row_iterator ri = featureMatrix.begin_row( sidx), re = featureMatrix.end_row( sidx);
			for (; ri != re; ++ri)
			{
				if (ri.val())
				{
					FeatureMatrix::const_row_iterator
						ci = featureInvMatrix.begin_row( ri.col()), ce = featureInvMatrix.end_row( ri.col());
					for (; ci != ce; ++ci)
					{
						if (sidx != ci.col())
						{
							outSimMatrix( sidx, ci.col()) = 1;
						}
					}
				}
			}
		}
		std::cerr << "test calculated feature assignments:" << std::endl;
		unsigned int nofMisses = 0;
		unsigned int nofFalsePositives = 0;
		unsigned int nofBadFalsePositives = 0;
		for (sidx=0; sidx != nofSamples; ++sidx)
		{
			for (std::size_t oidx=0; oidx != nofSamples; ++oidx)
			{
				double sim = simMatrix( sidx, oidx);
				unsigned char val = outSimMatrix( sidx, oidx);
				if (sidx != oidx)
				{
					if (sim >= 0.9)
					{
						if (val == 0)
						{
							nofMisses += 1;
#ifdef STRUS_LOWLEVEL_DEBUG
							std::cerr << "missed relation (" << sidx << "," << oidx << ") sim = " << sim << std::endl;
#endif
						}
					}
					else
					{
						if (val != 0)
						{
							nofFalsePositives += 1;
							if (sim < 0.85) nofBadFalsePositives += 1;
#ifdef STRUS_LOWLEVEL_DEBUG
							std::cerr << "false positive (" << sidx << "," << oidx << ") sim = " << sim << std::endl;
#endif
						}
					}
				}
			}
		}
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cerr << "output:" << std::endl << outSimMatrix.tostring() << std::endl;
		std::cerr << "expected:" << std::endl << expSimMatrix.tostring() << std::endl;
#endif
		std::cerr << "number of similarities = " << nofSimilarities << " reflexive = " << (nofSimilarities*2) << std::endl;
		std::cerr << "number of misses = " << nofMisses << std::endl;
		std::cerr << "false positives = " << nofFalsePositives << std::endl;
		std::cerr << "bad false positives = " << nofBadFalsePositives << std::endl;
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

