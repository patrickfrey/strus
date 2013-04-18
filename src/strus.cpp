#include "strus/storage.hpp"
#include "strus/iterator.hpp"
#include "strus/program.hpp"
#include <iostream>

int main( int argc, const char* argv[])
{
	if (argc <= 1)
	{
		std::cerr << "usage: strus <repository name>" << std::endl;
	}
	else
	{
		std::cerr << "using storage " << argv[1] << std::endl;
	}
}


