#include <memory>

int main()
{
	std::unique_ptr( new char( 5 ) );
	return 0;
}
