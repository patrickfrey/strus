#include "private/cstring.h"

const char* strus_memrchr( const char *s, int c, size_t n)
{
	while (n--)
	{
		const char *p = s+n;
		if (*p == (char)c)
		{
			return p;
		}
	}
	return 0;
}

