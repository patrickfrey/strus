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

#ifdef STRUS_LOWLEVEL_DEBUG
static void printSimHash( std::ostream& out, const std::vector<bool>& sh, std::size_t variations)
{
	std::vector<bool>::const_iterator si = sh.begin(), se = sh.end();
	for (std::size_t sidx=0; si != se; ++si,++sidx)
	{
		if (sidx && sidx % variations == 0)
		{
			out << "|";
			sidx = 0;
		}
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
		if (xidx)
		{
			fbuf << ", ";
		}
		fbuf << *xi;
	}
	out << "(" << fbuf.str() << ")";
}
#endif

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
	VectorSpaceModel( std::size_t dim_, std::size_t variations_, std::size_t width_)
		:m_dim(dim_),m_variations(variations_),m_width(width_)
		,m_modelMatrix( createModelMatrix( dim_, variations_))
	{
		std::size_t wi=0, we=width_;
		for (; wi != we; ++wi)
		{
			m_rotations.push_back( arma::randu<arma::mat>( m_dim, m_dim));
			if (std::abs( det( m_rotations.back())) < 0.01)
			{
				--wi;
				continue;
			}
		}
	}

	static arma::mat createModelMatrix( std::size_t dim_, std::size_t variations_)
	{
		if (dim_ <= 0 || variations_ <= 0)
		{
			throw std::runtime_error( "illegal dimension or variations");
		}
		if (dim_ < variations_)
		{
			throw std::runtime_error( "dimension must me two times bigger than variations");
		}
		double step = (float) dim_ / (float) variations_;
		arma::mat rt = arma::mat( variations_, dim_);
		std::size_t ri = 0, re = variations_;
		for (; ri != re; ++ri)
		{
			unsigned int ci = (unsigned int)(ri * step);
			unsigned int ce = (unsigned int)((ri+1) * step);
			if (ce > dim_) ce = dim_;
			if ((ri+1) == re) ce = dim_;
			rt.row( ri).fill( -1.0 / (dim_ - (ce - ci)));
			double val = 1.0 / (ce - ci);
			for (; ci < ce; ++ci)
			{
				rt( ri, ci) = val;
			}
		}
		return rt;
	}

	std::string tostring() const
	{
		std::ostringstream rt;
		rt << m_modelMatrix;
		return rt.str();
	}

	typedef std::vector<bool> SimHash;

	SimHash simHash( const arma::vec& vec) const
	{
		SimHash rt;
		if (m_dim != vec.size())
		{
			throw std::runtime_error( "vector must have dimension of model");
		}
		arma::vec res = m_modelMatrix * vec;
		arma::vec::const_iterator ri = res.begin(), re = res.end();
		for (; ri != re; ++ri)
		{
			rt.push_back( *ri >= 0.0);
		}
		return rt;
	}

	SimHash simHashW( const arma::vec& vec) const
	{
		SimHash rt;

		std::vector<arma::mat>::const_iterator ri = m_rotations.begin(), re = m_rotations.end();
		for (; ri != re; ++ri)
		{
			SimHash ww = simHash( (*ri) * vec);
			rt.insert( rt.end(), ww.begin(), ww.end());
		}
		return rt;
	}

	static SimHash sampleSimHash( const SimHash& sh, std::size_t variations, std::size_t width)
	{
		SimHash rt;
		enum {SumBits=8};
		if (variations * width != sh.size() && variations % SumBits != 0)
		{
			throw std::runtime_error( "parameters of simHashSample do not match");
		}
		std::size_t wi=0, we=width;
		for (; wi != we; ++wi)
		{
			std::size_t vi = 0, ve = variations / SumBits;
			for (; vi != ve; ++vi)
			{
				std::size_t cnt = 0;
				std::size_t oi=0, oe=SumBits;
				for (; oi != oe; ++oi)
				{
					if (sh[ wi * variations + vi * SumBits + oi]) ++cnt;
				}
				rt.push_back( (cnt > 4));
			}
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
	std::size_t m_width;
	arma::mat m_modelMatrix;
	std::vector<arma::mat> m_rotations;
};


#ifdef STRUS_LOWLEVEL_DEBUG
static VectorSpaceModel::SimHash diffSimHash( const VectorSpaceModel::SimHash& aa, const VectorSpaceModel::SimHash& bb)
{
	VectorSpaceModel::SimHash rt;
	VectorSpaceModel::SimHash::const_iterator ai = aa.begin(), ae = aa.end(), bi = bb.begin(), be = bb.end();
	for (; ai != ae && bi != be; ++ai,++bi)
	{
		rt.push_back( *ai ^ *bi);
	}
	return rt;
}
#endif


int main( int argc, const char** argv)
{
	try
	{
		initRandomNumberGenerator();
		double threshold_sim = 0.90;
		double accept_false_positive_sim = 0.80;
		unsigned int threshold_dist = 8;

		if (argc > 4)
		{
			std::cerr << "Usage: " << argv[0] << " [<threshold min sim>  <threshold max dist>  <threshold accept sim>]" << std::endl;
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
		if (argc >= 4)
		{
			bool error = false;
			if (argv[3][0] >= '0' && argv[3][0] <= '9')
			{
				accept_false_positive_sim = std::atof( argv[3]);
				error = (accept_false_positive_sim > 1.0);
			}
			else
			{
				error = true;
			}
			if (error) throw std::runtime_error( "floating point number between 0.0 and 1.0 expected as 3rd argument");
		}
		else
		{
			accept_false_positive_sim = threshold_sim - threshold_sim / 10;
		}
		std::cerr << "Threshold similarity = " << threshold_sim << std::endl;
		std::cerr << "Threshold accepted similarity = " << accept_false_positive_sim << std::endl;
		std::cerr << "Threshold distance = " << threshold_dist << std::endl;

		enum {Dim=300,Variations=64,Width=8,NofSampleVectors=200,NofSimilarVectors=2000};
		std::cout << "Build model "
			  << (unsigned int)Dim << "x" << (unsigned int)Dim
			  << " => " << (unsigned int)Variations << "*" << (unsigned int)Width << ":"
			  << std::endl;

		VectorSpaceModel model( Dim, Variations, Width);
#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "Model Matrix:" << std::endl << model.tostring() << std::endl;
#endif
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
			testshar.push_back( model.simHashW( *ti));
			testsimhashar.push_back( testshar.back());
		}

#ifdef STRUS_LOWLEVEL_DEBUG
		std::cout << "Results:" << std::endl;
		ti = testvectorar.begin(), te = testvectorar.end();
		std::size_t tidx = 0;
		for (; ti != te; ++ti,++tidx)
		{
			std::cout << "[" << tidx << "] ";
			printVector( std::cout, *ti);
			std::cout << " => ";
			printSimHash( std::cout, testshar[ tidx], Variations);
			std::cout << std::endl;
		}
#endif
		unsigned int all_candidate_count = 0;
		unsigned int positive_match_count = 0;
		unsigned int negative_match_count = 0;
		unsigned int missed_match_count = 0;
		unsigned int false_positive_count = 0;
		unsigned int accept_false_positive_count = 0;
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
					if (sim > accept_false_positive_sim)
					{
						accept_false_positive_count += 1;
					}
					else
					{
						tag = "false positive";
					}
				}
				if (tag)
				{
#ifdef STRUS_LOWLEVEL_DEBUG
					std::cout << tag << " [" << ai << "," << bi << "] SIM " << sim << " DIST " << dist << std::endl;
					printSimHash( std::cout, testshar[ ai], Variations); std::cout << std::endl;
					printSimHash( std::cout, testshar[ bi], Variations); std::cout << std::endl;
					std::cout << std::endl;
					std::cout << "DIFF = " << std::endl;
					printSimHash( std::cout, diffSimHash( testshar[ ai], testshar[ bi]), Variations);
					std::cout << std::endl;
#endif
#undef STRUS_DUMP_ERRORS
#ifdef STRUS_DUMP_ERRORS
					std::cout << "VECTOR A = ";
					printVector( std::cout, testvectorar[ ai]);
					std::cout << " => ";
					printSimHash( std::cout, testshar[ ai], Variations);
					std::cout << std::endl;
					std::cout << "VECTOR B = ";
					printVector( std::cout, testvectorar[ bi]);
					std::cout << " => ";
					printSimHash( std::cout, testshar[ bi], Variations);
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
		std::cout << "false_positive_count   = " << false_positive_count << " good " << accept_false_positive_count << " bad " << (false_positive_count - accept_false_positive_count) << std::endl;
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

