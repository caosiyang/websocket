/**
 *
 * filename: tools.h
 * summary:
 * author: caosiyang
 * email:  csy3228@gmail.com
 *
 */
#ifndef TOOLS_H
#define TOOLS_H

#include <iostream>
using namespace std;


#define LOG(format, args...) fprintf(stdout, format"\n", ##args)


inline uint16_t myhtons(uint16_t n) {
	return ((n & 0xff00) >> 8) | ((n & 0x00ff) << 8);
}


inline uint16_t myntohs(uint16_t n) {
	return ((n & 0xff00) >> 8) | ((n & 0x00ff) << 8);
}


inline uint32_t myhtonl(uint32_t n) {
	return ((n & 0xff000000) >> 24) | ((n & 0x00ff0000) >> 8) | ((n & 0x0000ff00) << 8) | ((n & 0x000000ff) << 24);
}


inline uint32_t myntohl(uint32_t n) {
	return ((n & 0xff000000) >> 24) | ((n & 0x00ff0000) >> 8) | ((n & 0x0000ff00) << 8) | ((n & 0x000000ff) << 24);
}


inline uint64_t myhtonll(uint64_t n) {
	return (uint64_t)myhtonl(n >> 32) | ((uint64_t)myhtonl(n) << 32);
}


inline uint64_t myntohll(uint64_t n) {
	return (uint64_t)myhtonl(n >> 32) | ((uint64_t)myhtonl(n) << 32);
}


#endif
