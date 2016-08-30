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

static void printVector( std::ostream& out, const std::vector<double>& vec)
{
	std::ostringstream fbuf;
	fbuf << std::setprecision(5) << std::fixed;
	std::vector<double>::const_iterator xi = vec.begin(), xe = vec.end();
	std::size_t xidx = 0;
	for (; xi != xe; ++xi,++xidx)
	{
		if (xidx) fbuf << ", ";
		fbuf << *xi;
	}
	out << "(" << fbuf.str() << ")";
}

std::vector<double> createSimilarVector( const std::vector<double>& vec, double maxCosSim)
{
	std::vector<double> rt( vec);
	arma::vec v_orig( vec);
	for (;;)
	{
		unsigned int idx = rand() % vec.size();
		double elem = rt[ idx];
		if ((rand() & 1) == 0)
		{
			elem -= elem / 10;
		}
		else
		{
			elem += elem / 10;
		}
		arma::vec v_new( rt);
		v_new[ idx] = elem;
		double cosSim = arma::norm_dot( v_orig, v_new);
		if (maxCosSim > cosSim)
		{
			return rt;
		}
		else
		{
			rt[ idx] = elem;
		}
	}
}

struct SimHashStruct
{
	SimHashStruct(){}
	SimHashStruct( const SimHashStruct& o)
		:ar1(o.ar1),ar2(o.ar2),un1(o.un1),un2(o.un2){}
	SimHashStruct( const std::vector<bool>& ar1_, const std::vector<bool>& ar2_, const std::vector<bool>& un1_, const std::vector<bool>& un2_)
		:ar1(ar1_),ar2(ar2_),un1(un1_),un2(un2_){}

	std::vector<bool> ar1;
	std::vector<bool> ar2;
	std::vector<bool> un1;
	std::vector<bool> un2;
};

static uint64_t bitset2uint64( const std::vector<bool>& ar)
{
	uint64_t rt = 0;
	std::vector<bool>::const_iterator ai = ar.begin(), ae = ar.end();
	for (unsigned int aidx=0; ai != ae && aidx < 64; ++ai,++aidx)
	{
		rt <<= 1;
		rt |= *ai ? 1:0;
	}
	return rt;
}

struct SimHashNum
{
	SimHashNum(){}
	SimHashNum( const SimHashNum& o)
		:val1(o.val1),val2(o.val2),un1(o.un1),un2(o.un2){}
	SimHashNum( const uint64_t& val1_, const uint64_t& val2_, const uint64_t& un1_, const uint64_t& un2_)
		:val1(val1_),val2(val2_),un1(un1_),un2(un2_){}
	SimHashNum( const SimHashStruct& st)
		:val1(bitset2uint64(st.ar1)),val2(bitset2uint64(st.ar2)),un1(bitset2uint64(st.un1)),un2(bitset2uint64(st.un2))
	{}

	unsigned int dist( const SimHashNum& o)
	{
		uint64_t diff = ((val1 ^ o.val1) & ~un1) | ((val2 ^ o.val2) & un1 & ~un2);
		return strus::BitOperations::bitCount( diff);
	}

	uint64_t val1;
	uint64_t val2;
	uint64_t un1;
	uint64_t un2;
};

class VectorSpaceModel
{
public:
	VectorSpaceModel( std::size_t dim_, std::size_t variations_)
		:m_dim(dim_),m_variations(variations_),m_matar()
	{
		if (m_dim <= 0 || m_variations <= 0)
		{
			throw std::runtime_error( "illegal dimension or variations");
		}
		std::size_t vi=0, ve=m_variations;
		for (; vi != ve; ++vi)
		{
			arma::mat mm( arma::randu<arma::mat>( m_dim, m_dim));
			double det = arma::det( mm);
			if (std::abs(det) < 0.01)
			{
				--vi;
				continue;
			}
			m_matar.push_back( mm);
		}
	}

	SimHashStruct simHash( const std::vector<double>& vec) const
	{
		if (m_dim != vec.size())
		{
			throw std::runtime_error( "vector must have dimension of model");
		}
		SimHashStruct rt;
		arma::vec vv( vec);
		std::vector<arma::mat>::const_iterator mi = m_matar.begin(), me = m_matar.end();
		for (; mi != me; ++mi)
		{
			arma::vec mapped_vv( *mi * vv);
			double maxval = mapped_vv[0]; 
			std::size_t maxidx = 0;
			double minval = mapped_vv[0]; 
			std::size_t minidx = 0;

			std::size_t di=1, de=m_dim;
			for (; di != de; ++di)
			{
				double val = mapped_vv[di];
				if (val > maxval)
				{
					maxidx = di;
					maxval = val;
				}
				if (val < minval)
				{
					minidx = di;
					minval = val;
				}
			}
			bool uncertain_max_flag = false;
			bool uncertain_min_flag = false;
			std::size_t ui,ue;
			if (maxidx < m_dim/2)
			{
				ui = m_dim/2;
				ue = m_dim;
			}
			else
			{
				ui = 0;
				ue = m_dim/2;
			}
			for (ui=0; ui != ue; ++ui)
			{
				if (mapped_vv[ui] + 10*std::numeric_limits<double>::epsilon() > maxval)
				{
					uncertain_max_flag = true;
				}
			}
			if (minidx < m_dim/2)
			{
				ui = m_dim/2;
				ue = m_dim;
			}
			else
			{
				ui = 0;
				ue = m_dim/2;
			}
			for (ui=0; ui != ue; ++ui)
			{
				if (mapped_vv[ui] - 10*std::numeric_limits<double>::epsilon() < minval)
				{
					uncertain_min_flag = true;
				}
			}
			rt.ar1.push_back( maxidx < m_dim/2);	// maximum in first half => true, in second half => false
			rt.ar2.push_back( minidx < m_dim/2);	// minimum in first half => true, in second half => false
			rt.un1.push_back( uncertain_max_flag);	// true, if the max decision is uncertain
			rt.un2.push_back( uncertain_min_flag);	// true, if the min decision is uncertain
		}
		return rt;
	}

private:
	std::size_t m_dim;
	std::size_t m_variations;
	std::vector<arma::mat> m_matar;
};



int main()
{
	try
	{
		initRandomNumberGenerator();
		double threshold_sim = 0.95;
		unsigned int threshold_dist = 8;
		unsigned int seek_dist = 20;

		enum {Dim=100,Variations=64,NofSampleVectors=100,NofSimilarVectors=1000};
		std::cout << "Build model "
			  << (unsigned int)Dim << "x" << (unsigned int)Dim
			  << " => " << (unsigned int)NofSampleVectors << ":" << std::endl;
	
		VectorSpaceModel model( Dim, Variations);
		std::vector<std::vector<double> > testvectorar;
		std::vector<SimHashStruct> testsimhashar;

		std::cout << "Build " << (unsigned int)NofSampleVectors << " samples:" << std::endl;
		unsigned int vi=0,ve=NofSampleVectors;
		for (; vi != ve; ++vi)
		{
			std::vector<double> rv;
			std::size_t di=0, de=Dim;
			for (; di != de; ++di)
			{
				rv.push_back( (double)std::rand() / RAND_MAX);
			}
			testvectorar.push_back( rv);
		}
		std::cout << "Build " << (unsigned int)NofSimilarVectors << " similar vector for samples:" << std::endl;
		vi=0; ve=NofSimilarVectors;
		for (; vi != ve; ++vi)
		{
			std::size_t origidx = rand() % NofSampleVectors;
			double sim = 0.90 + (rand() % 99) * 0.001;
			std::vector<double> rv = createSimilarVector( testvectorar[origidx], sim);
			testvectorar.push_back( rv);
		}
	
		std::cout << "Build " << testvectorar.size() << " sim hashes:" << std::endl;
		std::vector<std::vector<double> >::const_iterator ti = testvectorar.begin(), te = testvectorar.end();
		for (; ti != te; ++ti)
		{
			testsimhashar.push_back( model.simHash( *ti));
		}

		std::cout << "Results:" << std::endl;
		ti = testvectorar.begin(), te = testvectorar.end();
		std::size_t tidx = 0;
		for (; ti != te; ++ti,++tidx)
		{
			std::cout << "[" << tidx << "] ";
			printVector( std::cout, *ti);
			std::cout << " => max ";
			printSimHash( std::cout, testsimhashar[ tidx].ar1);
			std::cout << " min ";
			printSimHash( std::cout, testsimhashar[ tidx].ar2);
			std::cout << " uncertain max ";
			printSimHash( std::cout, testsimhashar[ tidx].un1);
			std::cout << " uncertain min ";
			printSimHash( std::cout, testsimhashar[ tidx].un2);
			std::cout << std::endl;
		}
		std::vector<arma::vec> var;
		std::vector<SimHashNum> sar;
		ti = testvectorar.begin(), te = testvectorar.end();
		for (; ti != te; ++ti)
		{
			var.push_back( *ti);
		}
		std::vector<SimHashStruct>::const_iterator si = testsimhashar.begin(), se = testsimhashar.end();
		for (; si != se; ++si)
		{
			sar.push_back( SimHashNum( *si));
		}
		unsigned int all_candidate_count = 0;
		unsigned int missed_dist = 0;
		unsigned int missed_sim = 0;
		unsigned int all_sim_count = 0;
		unsigned int ok_pos_count = 0;
		unsigned int ok_neg_count = 0;
		unsigned int seek_dist_all_count = 0;
		unsigned int seek_dist_pos_count = 0;

		unsigned int ai=0,ae=testvectorar.size();
		for (; ai<ae; ++ai)
		{
			unsigned int bi=0,be=testvectorar.size();
			for (; bi<be; ++bi)
			{
				all_candidate_count += 1;
				double sim = arma::norm_dot( var[ai], var[bi]);
				if (sim > threshold_sim)
				{
					all_sim_count += 1;
				}
				unsigned int dist = sar[ai].dist( sar[bi]);
	
				if (seek_dist <= dist)
				{
					seek_dist_all_count += 1;
					if (sim > threshold_sim)
					{
						seek_dist_pos_count += 1;
					}
				}
				if (sim > threshold_sim && dist > threshold_dist)
				{
					std::cout << "similar but not candidate [" << ai << "," << bi << "] SIM " << sim << " DIST " << dist << std::endl;
					std::cout << "VECTOR A = ";
					printVector( std::cout, testvectorar[ ai]);
					std::cout << " max ";
					printSimHash( std::cout, testsimhashar[ ai].ar1);
					std::cout << " min ";
					printSimHash( std::cout, testsimhashar[ ai].ar2);
					std::cout << " uncertain max ";
					printSimHash( std::cout, testsimhashar[ ai].un1);
					std::cout << " uncertain min ";
					printSimHash( std::cout, testsimhashar[ ai].un2);
					std::cout << std::endl;
					std::cout << "VECTOR B = ";
					printVector( std::cout, testvectorar[ bi]);
					std::cout << " max ";
					printSimHash( std::cout, testsimhashar[ bi].ar1);
					std::cout << " min ";
					printSimHash( std::cout, testsimhashar[ bi].ar2);
					std::cout << " uncertain max ";
					printSimHash( std::cout, testsimhashar[ bi].un1);
					std::cout << " uncertain min ";
					printSimHash( std::cout, testsimhashar[ bi].un2);
					std::cout << std::endl;
					missed_dist += 1;
				}
				if (sim < threshold_sim && dist <= threshold_dist)
				{
					std::cout << "candidate but not similar [" << ai << "," << bi << "] SIM " << sim << " DIST " << dist << std::endl;
					missed_sim += 1;
				}
				if (sim > threshold_sim && dist <= threshold_dist)
				{
					std::cout << "OK [" << ai << "," << bi << "] SIM " << sim << " DIST " << dist << std::endl;
					ok_pos_count += 1;
				}
				if (sim < threshold_sim && dist > threshold_dist)
				{
					ok_neg_count += 1;
				}
			}
		}
		std::cout << "ok pos count = " << ok_pos_count << std::endl;
		std::cout << "ok neg count = " << ok_neg_count << std::endl;
		std::cout << "all sim count = " << all_sim_count << std::endl;
		std::cout << "candidate but not similar = " << missed_sim << std::endl;
		std::cout << "similar but not candidate = " << missed_dist << std::endl;
		std::cout << "seek all count = " << seek_dist_all_count << std::endl;
		std::cout << "seek pos count = " << seek_dist_pos_count << std::endl;
		std::cout << "all candidate count = " << all_candidate_count << std::endl;
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

