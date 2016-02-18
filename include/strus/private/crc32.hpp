#ifndef _STRUS_UTILS_CRC32_HPP_INCLUDED
#define _STRUS_UTILS_CRC32_HPP_INCLUDED
#include <utility>
#include <stdint.h>

namespace strus {
namespace utils {

class Crc32
{
public:
	static uint32_t calc( const char* blk, std::size_t blksize);
	static uint32_t calc( const char* blk);
};

}}
#endif

