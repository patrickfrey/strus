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
	
		double det_a = arma::det( A);
		std::cout << "determinant of A:" << det_a << std::endl;
		double det_b = arma::det( B);
		std::cout << "determinant of B:" << det_b << std::endl;
		double det_c = arma::det( C);
		std::cout << "determinant of C:" << det_c << std::endl;

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

