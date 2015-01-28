SILLY IDEA, BUT I LEARNED. THE CODE IS NOT WORKING

static inline int utf8CharCompare( const unsigned char* chr1, const unsigned char* chr2, uint32_t chrsize)
{
#ifdef __x86_64__
	char rt_eq;
	char rt_g;
	asm(
		" movdqu (%2), %%xmm0 \n"
		" movdqu (%3), %%xmm1 \n"
		" pcmpistri $0x18, %%xmm1, %%xmm0 \n"		/// $18 = EQUAL_EACH + NEGATIVE_POLARITY
		" cmp %%ecx, %4 \n"				/// compare with chr length
		" seta %0 \n"					/// if the chr length is bigger than the first found inequality index, then the characters are not equal
		" pcmpgtd %%xmm1, %%xmm0 \n"			/// subtract all bytes of the character
		" pmovmskb %%xmm0, %%ecx \n"			/// move sign bits of result into mask in ecx (cx)
		" bsf %%ecx, %%ecx \n"				/// find first occurrence of sign bit in mask
		" cmp %%ecx, %4 \n"				/// compare with chr length
		" seta %1 \n"					/// if the chr length is bigger than the first found "greater than index", then the first character is not bigger
		: "=r"(rt_eq), "=r"(rt_g)
		: "r"(chr1), "r"(chr2), "r"(chrsize)
		: "xmm0", "xmm1", "ecx"
	);
	return -1 + rt_eq + (rt_g<<1);
#else
	return std::memcmp( chr1, chr2, chrsize);
#endif
}


TEST

static void utf8CharCompareTest( int expectedResult, const strus::Index& chr1, const strus::Index& chr2)
{
	std::string str1;
	std::string str2;
	strus::packIndex( str1, chr1);
	strus::packIndex( str2, chr2);

	int res = BitOperations::utf8CharCompare(
			(const unsigned char*)str1.c_str(),
			(const unsigned char*)str2.c_str(), str1.size());
	if (res != expectedResult)
	{
		std::cerr << "expected "<< expectedResult << ", got " << res << " in UTF-8 character compare for " << chr1 << " and " << chr2 << std::endl;
		throw std::runtime_error( "UTF-8 character compare test failed");
	}
	std::cerr << "tested UTF-8 character compare for " << chr1 << " " << chr2 << std::endl;
}

static void testUtf8CharCompare()
{
	strus::Index ii = 1;
	for (; ii<= 100000; ++ii)
	{
		Index chr1 = RANDINT(1,(1<<31));
		Index chr2 = RANDINT(1,(1<<31));
		if (RANDINT(1,10) == 3)
		{
			chr2 = chr1;
		}
		int expectedResult = chr1==chr2?0:(chr1<chr2?-1:+1);
		utf8CharCompareTest( expectedResult, chr1, chr2);
	}
}

