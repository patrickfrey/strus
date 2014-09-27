#include "indexPacker.hpp"
#include <map>
#include <limits>
#include <stdexcept>

#define B11111111 0xFF
#define B01111111 0x7F
#define B00111111 0x3F
#define B00011111 0x1F
#define B00001111 0x0F
#define B00000111 0x07
#define B00000011 0x03
#define B00000001 0x01
#define B00000000 0x00
#define B10000000 0x80
#define B11000000 0xC0
#define B11100000 0xE0
#define B11110000 0xF0
#define B11111000 0xF8
#define B11111100 0xFC
#define B11111110 0xFE

#define B11011111 (B11000000|B00011111)
#define B11101111 (B11100000|B00001111)
#define B11110111 (B11110000|B00000111)
#define B11111011 (B11111000|B00000011)
#define B11111101 (B11111100|B00000001)

using namespace strus;

struct CharLengthTab
{
	char m_ar[256];

	unsigned char operator[]( unsigned char ii) const
	{
		return m_ar[ii];
	}

	void fill( unsigned int start, unsigned int end, unsigned char value)
	{
		for (unsigned int ii=start; ii<=end; ++ii) m_ar[ii] = value;
	}

	CharLengthTab()
	{
		fill( B00000000,B01111111,1);
		fill( B11000000,B11011111,2);
		fill( B11100000,B11101111,3);
		fill( B11110000,B11110111,4);
		fill( B11111000,B11111011,5);
		fill( B11111100,B11111101,6);
		fill( B11111110,B11111110,7);
		fill( B11111111,B11111111,8);
	}
};

static CharLengthTab g_charlentable;

static unsigned int utf8decode( const char* itr)
{
	unsigned int charsize = g_charlentable[ *itr];
	unsigned int res = (unsigned char)*itr;
	if (res > 127)
	{
		unsigned int gg = charsize-2;
		if (gg < 0) return -1;

		res = ((unsigned char)*itr)&(B00011111>>gg);
		for (unsigned int ii=0; ii<=gg; ii++)
		{
			unsigned char xx = (unsigned char)itr[ii+1];
			res = (res<<6) | (xx & B00111111);
			if ((unsigned char)(xx & B11000000) != B10000000)
			{
				return -1;
			}
		}
	}
	return res;
}

static void utf8encode( std::string& buf, unsigned int chr)
{
	unsigned int rt;
	if (chr <= 127)
	{
		buf.push_back( (char)(unsigned char)chr);
		return;
	}
	unsigned int pp,sf;
	for (pp=1,sf=5; pp<5; pp++,sf+=5)
	{
		if (chr < (unsigned int)((1<<6)<<sf)) break;
	}
	rt = pp+1;
	unsigned char HB = (unsigned char)(B11111111 << (8-rt));
	unsigned char shf = (unsigned char)(pp*6);
	unsigned int ii;
	buf.push_back( (char)(((unsigned char)(chr >> shf) & (~HB >> 1)) | HB));
	for (ii=1,shf-=6; ii<=pp; shf-=6,ii++)
	{
		buf.push_back( (char)(unsigned char) (((chr >> shf) & B00111111) | B10000000));
	}
}

static unsigned int unpackInt32_( const char* itr, const char* end)
{
	int ii;
	int nn = g_charlentable[ *itr];
	char buf[8];
	for (ii=0; itr != end && ii < nn; ++itr,++ii)
	{
		buf[ii] = *itr;
	}
	if (ii < nn) throw std::runtime_error( "corrupt data");
	unsigned int rt = utf8decode( buf);
	if (rt > 0x7fffFFFFU) throw std::runtime_error( "corrupt data");
	return rt;
}

Index strus::unpackIndex( const char*& itr, const char* end)
{
	if (*itr == (char)B11111111)
	{
		++itr;
		Index hi = unpackInt32_( itr, end);
		Index lo = unpackInt32_( itr, end);
		return (hi << 31) + lo;
	}
	else
	{
		return (Index)unpackInt32_( itr, end);
	}
}

void strus::packIndex( std::string& buf, const Index& idx)
{
	if (idx > 0x7fffFFFFU)
	{
		buf.push_back( (char)(unsigned char)B11111111);
		Index hi = idx >> 31;
		Index lo = idx & 0x7fffFFFFU;
		if (hi > 0x7fffFFFFU)
		{
			throw std::runtime_error( "index out of range");
		}
		utf8encode( buf, (unsigned int)hi);
		utf8encode( buf, (unsigned int)lo);
	}
	else
	{
		utf8encode( buf, (unsigned int)idx);
	}
}

