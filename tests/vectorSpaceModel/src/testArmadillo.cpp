#include <iostream>
#define ARMA_DONT_USE_WRAPPER
#include <armadillo>

int main()
{
	arma::mat A = arma::randu<arma::mat>(5,5);
	arma::mat B = arma::randu<arma::mat>(5,5);

	std::cout << "A:" << std::endl;
	std::cout << A << std::endl;
	std::cout << "B:" << std::endl;
	std::cout << B << std::endl;

	std::cout << "A * B transposed:" << std::endl;
	std::cout << A * B.t() << std::endl;

	double det_a = arma::det( A);
	std::cout << "determinant of A:" << det_a << std::endl;
	double det_b = arma::det( B);
	std::cout << "determinant of B:" << det_b << std::endl;

	return 0;
}

