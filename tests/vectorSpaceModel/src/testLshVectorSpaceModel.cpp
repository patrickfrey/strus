/*
 * Copyright (c) 2016 Patrick P. Frey
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
/// \brief Test program

#include "private/bitOperations.hpp"
#include <armadillo>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstdlib>
#include <cmath>
#include <limits>


static void initRandomNumberGenerator()
{
	time_t nowtime;
	struct tm* now;

	::time( &nowtime);
	now = ::localtime( &nowtime);

	unsigned int seed = (now->tm_year+10000) + (now->tm_mon+100) + (now->tm_mday+1);
	std::srand( seed+2);
}

static void printSimHash( std::ostream& out, const std::vector<bool>& sh)
{
	std::vector<bool>::const_iterator si = sh.begin(), se = sh.end();
	for (; si != se; ++si)
	{
		out << (*si ? "1":"0");
	}
}

static void printVector( std::ostream& out, const arma::vec& vec)
{
	std::ostringstream fbuf;
	fbuf << std::setprecision(5) << std::fixed;
	arma::vec::const_iterator xi = vec.begin(), xe = vec.end();
	std::size_t xidx = 0;
	for (; xi != xe; ++xi,++xidx)
	{
		if (xidx) fbuf << ", ";
		fbuf << *xi;
	}
	out << "(" << fbuf.str() << ")";
}

arma::vec createSimilarVector( const arma::vec& vec, double maxCosSim)
{
	arma::vec rt( vec);
	for (;;)
	{
		unsigned int idx = rand() % vec.size();
		double elem = rt[ idx];
		if ((rand() & 1) == 0)
		{
			elem -= elem / 10;
			if (elem < 0.0) continue;
		}
		else
		{
			elem += elem / 10;
		}
		double oldelem = rt[ idx];
		rt[ idx] = elem;
		double cosSim = arma::norm_dot( vec, rt);
		if (maxCosSim > cosSim)
		{
			rt[ idx] = oldelem;
			return rt;
		}
	}
}

class VectorSpaceModel
{
public:
	VectorSpaceModel( std::size_t dim_, std::size_t variations_)
		:m_dim(dim_),m_variations(variations_),m_modelMatrix()
	{
		if (m_dim <= 0 || m_variations <= 0)
		{
			throw std::runtime_error( "illegal dimension or variations");
		}
		if (m_dim < m_variations)
		{
			throw std::runtime_error( "dimension must me two times bigger than variations");
		}
		do
		{
			m_modelMatrix = 2.0 * arma::randu<arma::mat>( m_dim, m_dim)
					- arma::mat( m_dim, m_dim).ones();
		}
		while (std::fabs(arma::det(m_modelMatrix)) <= 0.01);

		m_simhashMatrix = m_modelMatrix.rows( 0, m_variations-1);
	}

	typedef std::vector<bool> SimHash;

	// See https://en.wikipedia.org/wiki/Locality-sensitive_hashing
	// (random projection method of LSH due to Moses Charikar)
	// Charikar, Moses S. (2002). "Similarity Estimation Techniques from Rounding Algorithms". 
	// Proceedings of the 34th Annual ACM Symposium on Theory of Computing. pp. 380–388.
	// doi:10.1145/509907.509965.
	SimHash simHash( const arma::vec& vec) const
	{
		std::vector<bool> rt;
		if (m_dim != vec.size())
		{
			throw std::runtime_error( "vector must have dimension of model");
		}
		arma::vec res = m_simhashMatrix * vec;
		arma::vec::const_iterator ri = res.begin(), re = res.end();
		for (; ri != re; ++ri)
		{
			rt.push_back( *ri >= 0.0);
		}
		return rt;
	}

	struct SimHashStruct
	{
		SimHashStruct( const SimHashStruct& o)
			:ar(o.ar){}
		SimHashStruct( const SimHash& o)
			:ar()
		{
			unsigned int idx = 0;
			for (; idx < o.size(); idx+=64)
			{
				ar.push_back( bitset2uint64( o, idx));
			}
		}

		unsigned int dist( const SimHashStruct& o) const
		{
			unsigned int rt = 0;
			std::vector<uint64_t>::const_iterator ai = ar.begin(), ae = ar.end();
			std::vector<uint64_t>::const_iterator oi = o.ar.begin(), oe = o.ar.end();
			for (; oi != oe && ai != ae; ++oi,++ai)
			{
				rt += strus::BitOperations::bitCount( *ai ^ *oi);
			}
			for (; oi != oe; ++oi)
			{
				rt += strus::BitOperations::bitCount( *oi);
			}
			for (; ai != ae; ++ai)
			{
				rt += strus::BitOperations::bitCount( *ai);
			}
			return rt;
		}

		static uint64_t bitset2uint64( const std::vector<bool>& ar, unsigned int idx)
		{
			uint64_t rt = 0;
			std::vector<bool>::const_iterator ai = ar.begin() + idx, ae = ar.end();
			for (unsigned int aidx=0; ai != ae && aidx < 64; ++ai,++aidx)
			{
				rt <<= 1;
				rt |= *ai ? 1:0;
			}
			return rt;
		}

		std::vector<uint64_t> ar;
	};

private:
	std::size_t m_dim;
	std::size_t m_variations;
	arma::mat m_modelMatrix;
	arma::mat m_simhashMatrix;
};



int main( int argc, const char** argv)
{
	try
	{
		initRandomNumberGenerator();
		double threshold_sim = 0.90;
		unsigned int threshold_dist = 8;

		if (argc > 3)
		{
			std::cerr << "Usage: " << argv[0] << " [<threshold min sim>  <threshold max dist>]" << std::endl;
			throw std::runtime_error( "too many arguments (maximum 2 expected)");
		}
		if (argc >= 3)
		{
			bool error = false;
			if (argv[2][0] >= '0' && argv[2][0] <= '9')
			{
				threshold_dist = std::atoi( argv[2]);
			}
			else
			{
				error = true;
			}
			if (error) throw std::runtime_error( "number (non negative integer) expected as 2nd argument");
		}
		if (argc >= 2)
		{
			bool error = false;
			if (argv[1][0] >= '0' && argv[1][0] <= '9')
			{
				threshold_sim = std::atof( argv[1]);
				error = (threshold_sim > 1.0);
			}
			else
			{
				error = true;
			}
			if (error) throw std::runtime_error( "floating point number between 0.0 and 1.0 expected as 1st argument");
		}
		std::cerr << "Threshold similarity = " << threshold_sim << std::endl;
		std::cerr << "Threshold distance = " << threshold_dist << std::endl;

		enum {Dim=300,Variations=128,NofSampleVectors=100,NofSimilarVectors=1000};
		std::cout << "Build model "
			  << (unsigned int)Dim << "x" << (unsigned int)Dim
			  << " => " << (unsigned int)NofSampleVectors << ":" << std::endl;

		VectorSpaceModel model( Dim, Variations);
		std::vector<arma::vec> testvectorar;
		std::vector<VectorSpaceModel::SimHash> testshar;
		std::vector<VectorSpaceModel::SimHashStruct> testsimhashar;

		std::cout << "Build " << (unsigned int)NofSampleVectors << " samples:" << std::endl;
		unsigned int vi=0,ve=NofSampleVectors;
		for (; vi != ve; ++vi)
		{
			testvectorar.push_back( arma::randu<arma::vec>( Dim));
		}
		std::cout << "Build " << (unsigned int)NofSimilarVectors << " similar vector for samples:" << std::endl;
		vi=0; ve=NofSimilarVectors;
		for (; vi != ve; ++vi)
		{
			std::size_t origidx = rand() % NofSampleVectors;
			double sim = 0.90 + (rand() % 99) * 0.001;
			testvectorar.push_back( createSimilarVector( testvectorar[ origidx], sim));
		}

		std::cout << "Build " << testvectorar.size() << " sim hashes:" << std::endl;
		std::vector<arma::vec>::const_iterator ti = testvectorar.begin(), te = testvectorar.end();
		for (; ti != te; ++ti)
		{
			testshar.push_back( model.simHash( *ti));
			testsimhashar.push_back( testshar.back());
		}

		std::cout << "Results:" << std::endl;
		ti = testvectorar.begin(), te = testvectorar.end();
		std::size_t tidx = 0;
		for (; ti != te; ++ti,++tidx)
		{
			std::cout << "[" << tidx << "] ";
			printVector( std::cout, *ti);
			std::cout << " => ";
			printSimHash( std::cout, testshar[ tidx]);
			std::cout << std::endl;
		}

		unsigned int all_candidate_count = 0;
		unsigned int positive_match_count = 0;
		unsigned int negative_match_count = 0;
		unsigned int missed_match_count = 0;
		unsigned int false_positive_count = 0;
		unsigned int nof_lsh_matches = 0;
		unsigned int nof_sim_matches = 0;

		unsigned int ai=0,ae=testvectorar.size();
		for (; ai<ae; ++ai)
		{
			unsigned int bi=0,be=testvectorar.size();
			for (; bi<be; ++bi)
			{
				double sim = arma::norm_dot( testvectorar[ai], testvectorar[bi]);
				unsigned int dist = testsimhashar[ ai].dist( testsimhashar[ bi]);

				bool is_lsh_match = (dist <= threshold_dist);
				bool is_sim_match = (sim > threshold_sim);

				nof_lsh_matches += is_lsh_match ? 1:0;
				nof_sim_matches += is_sim_match ? 1:0;
				const char* tag = 0;
				all_candidate_count += 1;

				if (is_lsh_match && is_sim_match)
				{
					positive_match_count += 1;//Ok
				}
				else if (!is_lsh_match && !is_sim_match)
				{
					negative_match_count += 1;//Ok
				}
				else if (!is_lsh_match && is_sim_match)
				{
					missed_match_count += 1;
					tag = "missed match";
				}
				else if (is_lsh_match && !is_sim_match)
				{
					false_positive_count += 1;
					tag = "false positive";
				}
				if (tag)
				{
					std::cout << tag << " [" << ai << "," << bi << "] SIM " << sim << " DIST " << dist << std::endl;
#undef STRUS_DUMP_ERRORS
#ifdef STRUS_DUMP_ERRORS
					std::cout << "VECTOR A = ";
					printVector( std::cout, testvectorar[ ai]);
					std::cout << " => ";
					printSimHash( std::cout, testshar[ ai]);
					std::cout << std::endl;
					std::cout << "VECTOR B = ";
					printVector( std::cout, testvectorar[ bi]);
					std::cout << " => ";
					printSimHash( std::cout, testshar[ bi]);
					std::cout << std::endl;
#endif
				}
			}
		}
		std::cout << "all_candidate_count    = " << all_candidate_count << std::endl;
		std::cout << "similarity_match_count = " << nof_sim_matches << std::endl;
		std::cout << "lsh_match_count        = " << nof_lsh_matches << std::endl;
		std::cout << "positive_match_count   = " << positive_match_count << std::endl;
		std::cout << "negative_match_count   = " << negative_match_count << std::endl;
		std::cout << "missed_match_count     = " << missed_match_count << std::endl;
		std::cout << "false_positive_count   = " << false_positive_count << std::endl;
		return 0;
	}
	catch (const std::runtime_error& err)
	{
		std::cerr << "error: " << err.what() << std::endl;
		return -1;
	}
	catch (const std::bad_alloc& )
	{
		std::cerr << "out of memory" << std::endl;
		return -2;
	}
	catch (const std::logic_error& err)
	{
		std::cerr << "error: " << err.what() << std::endl;
		return -3;
	}
}

