#pragma once

/*************************************************************************
 * Copyright © 2025 Hudyaev Alexy <hudyaev.alexy@gmail.com>
 * This file is part of xmsm.
 * Distributed under the GNU Affero General Public License.
 * See accompanying file copying (at the root of this repository)
 * or <http://www.gnu.org/licenses/> for details
 *************************************************************************/

#if __has_include(<stdint.h>)
#include <stdint.h>
#else
//WARNING: the following types adapted to wasm32
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
typedef signed long long __int64_t;
typedef unsigned long long __uint64_t;
typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
#endif

#if __has_include(<bit>)
#include <bit>
namespace xmsm::details { constexpr bool is_little_endian(){return std::endian::native == std::endian::little;} }
#else
//WARNING: use little endian by default: wasm32 use the little
//         we can use this commands to get info about byte order:
//         clang++ -dM -E -x c++ /dev/null | grep BYTE_ORDER
//         g++     -dM -E -x c++ /dev/null | grep BYTE_ORDER
namespace xmsm::details { constexpr bool is_little_endian(){return true;} }
#endif

namespace xmsm {

#if __cplusplus > 202302L
constexpr auto hash32(const void* src, int len, uint32_t seed) {
#else
constexpr auto hash32(const char* src, int len, uint32_t seed) {
#endif
  // murmurhash from https://github.com/aappleby/smhasher/blob/0ff96f7835817a27d0487325b6c16033e2992eb5/src/MurmurHash2.cpp#L37
  const uint32_t m = 0x5bd1e995;
  const int r = 24;
  uint32_t h = seed ^ len;
  const char* data = (const char*)src;
  while(len >= 4)
  {
    uint32_t k = [&]{if !consteval { return *(uint32_t*)data; } else {
      if constexpr(details::is_little_endian()) return (uint32_t)(data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24));
      else return (uint32_t)(data[3] + (data[2] << 8) + (data[1] << 16) + (data[0] << 24));
    } }();

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  switch(len)
  {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
            h *= m;
  }

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.
  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}
constexpr auto hash64(const void* src, int len, uint64_t seed) {
  // murmurhash from https://github.com/aappleby/smhasher/blob/0ff96f7835817a27d0487325b6c16033e2992eb5/src/MurmurHash2.cpp#L96
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

  //TODO: it doesn't work in compile time due src was const char* (use if consteval hack...)
  const uint64_t * data = (const uint64_t *)src;
  const uint64_t * end = data + (len/8);

  while(data != end)
  {
    uint64_t k = *data++;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char * data2 = (const unsigned char*)data;

  switch(len & 7)
  {
  case 7: h ^= uint64_t(data2[6]) << 48;
  case 6: h ^= uint64_t(data2[5]) << 40;
  case 5: h ^= uint64_t(data2[4]) << 32;
  case 4: h ^= uint64_t(data2[3]) << 24;
  case 3: h ^= uint64_t(data2[2]) << 16;
  case 2: h ^= uint64_t(data2[1]) << 8;
  case 1: h ^= uint64_t(data2[0]);
          h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

}
