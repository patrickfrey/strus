#include <chrono>
#include <iostream>
#include <iomanip>

int main()
{
	auto start = std::chrono::system_clock::now( );
	auto end = std::chrono::system_clock::now( );
	std::chrono::duration<double> duration = end - start;

	std::cout << "duration: " << std::fixed << std::setprecision( 6 ) << duration.count( ) << std::endl;

	if( duration.count( ) > 10.0 ) {
		return 1;
	}

	return 0;
}
