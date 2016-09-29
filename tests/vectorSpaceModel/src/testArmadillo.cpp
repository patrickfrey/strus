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

int main()
{
	try
	{
		initRandomNumberGenerator();
	
		arma::mat A = arma::randu<arma::mat>(5,5);
		arma::mat B = arma::randu<arma::mat>(5,5);
		arma::mat C = 2.0 * arma::randu<arma::mat>( 5, 5) - arma::mat( 5, 5).ones();

		std::cout << "A:" << std::endl;
		std::cout << A << std::endl;
		std::cout << "B:" << std::endl;
		std::cout << B << std::endl;
		std::cout << "C:" << std::endl;
		std::cout << C << std::endl;

		std::cout << "A * B transposed:" << std::endl;
		std::cout << A * B.t() << std::endl;
	
		unsigned int rank_a = arma::rank( A, std::numeric_limits<float>::epsilon());
		std::cout << "rank of A:" << rank_a << std::endl;
		unsigned int rank_b = arma::rank( B, std::numeric_limits<float>::epsilon());
		std::cout << "rank of B:" << rank_b << std::endl;
		unsigned int rank_c = arma::rank( C, std::numeric_limits<float>::epsilon());
		std::cout << "rank of C:" << rank_c << std::endl;

		{
			arma::sp_mat A(5,6);
			arma::sp_mat B(6,5);

			A(0,0) = 1;
			A(1,0) = 2;

			B(0,0) = 3;
			B(0,1) = 4;

			arma::sp_mat C = 2*B;

			arma::sp_mat D = A*C;

			// batch insertion of two values at (5, 6) and (9, 9)
			arma::umat locations1( 2,3);
			locations1(0,0) = 900;
			locations1(0,1) = 901;
			locations1(0,2) = 903;
			locations1(1,0) = 910;
			locations1(1,1) = 911;
			locations1(1,2) = 912;

			arma::umat locations2;
			locations2 << 900 << 901 << 903 << arma::endr
				   << 910 << 911 << 913 << arma::endr;

			arma::vec values;
			values << 1.1 << 1.2 << 1.3 << arma::endr;

			arma::sp_mat X( locations1, values);
			arma::sp_mat Y( locations1, values);
			std::cout << X << std::endl;
			std::cout << Y << std::endl;
		}
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

