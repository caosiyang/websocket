#include "md5.h"


const uint32_t MD5::CV[4] = {
	0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476
};


const uint32_t MD5::R[64] = {
	7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
	5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
	4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
	6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};


MD5::MD5() {
	//initialize T[]
	for (int i = 1; i <= 64; ++i) {
		long long ti = (long long)(pow(2, 32) * sin(i));
		if (ti < 0) {
			ti = 0 - ti;
		}
		T[i - 1] = ti;
	}
}


MD5::~MD5() {
}


string MD5::calc(const char *str) {
	return calc(str, strlen(str));
}


string MD5::calc(const char *buf, const int32_t len) {
	char *outbuf = NULL;
	int32_t outlen = 0;
	prepare(buf, len, outbuf, outlen);
	assert(outlen > 0 && outlen % 64 == 0);
	
	uint32_t A = CV[0];
	uint32_t B = CV[1];
	uint32_t C = CV[2];
	uint32_t D = CV[3];
	
	for (int32_t i = 0; i < outlen / 64; ++i) {
		uint32_t X[16] = {0};
		for (int32_t j = 0; j < 16; ++j) {
			X[j] = *(uint32_t*)(outbuf + (64 * i + 4 * j));
		}

		uint32_t a = A;
		uint32_t b = B;
		uint32_t c = C;
		uint32_t d = D;
		
		for (int i = 0; i < 64; ++i) {
			int32_t g = 0;
			if (i >= 0 && i <= 15) {
				g = i;
				a = (uint32_t)(b + rotate_left(a + F(b, c, d) + X[g] + T[i], R[i]));
			} else if (i >= 16 && i <= 31) {
				g = (i * 5 + 1) % 16;
				a = (uint32_t)(b + rotate_left(a + G(b, c, d) + X[g] + T[i], R[i]));
			} else if (i >= 32 && i <= 47) {
				g = (i * 3 + 5) % 16;
				a = (uint32_t)(b + rotate_left(a + H(b, c, d) + X[g] + T[i], R[i]));
			} else if (i >= 48 && i <= 63) {
				g = (i * 7) % 16;
				a = (uint32_t)(b + rotate_left(a + I(b, c, d) + X[g] + T[i], R[i]));
			}
			
			//loop swap
			uint32_t temp = d;
			d = c;
			c = b;
			b = a;
			a = temp;
		}

		A = A + a;
		B = B + b;
		C = C + c;
		D = D + d;
	}

	return _gen_md5_str(A, B, C, D);
}


inline uint32_t MD5::rotate_left(uint32_t num, uint32_t n) const {
	return (num << n) | (num >> (32 - n % 32));
}


string MD5::_gen_md5_str(uint32_t a, uint32_t b, uint32_t c, uint32_t d) const {
	char res[33] = {0};
	sprintf(res, "%08x%08x%08x%08x", _reverse(a), _reverse(b), _reverse(c), _reverse(d));
	return res;
}


inline uint32_t MD5::_reverse(const uint32_t n) const {
	return ((n & 0x000000ff) << 24) | ((n & 0x0000ff00) << 8) | ((n & 0x00ff0000) >> 8) | ((n & 0xff000000) >> 24);
}


void MD5::prepare(const char *buf, int32_t len, char *&outbuf, int32_t &outlen) {
	int32_t x = len * 8 % 512;
	int32_t bit_to_fill = (x < 448) ? (448 - x) : (512 - x + 448);
	char c1 = 1, c2 = 0;
	c1 = c1 << 7;

	if (bit_to_fill % 8 == 0) {
		int32_t byte_to_fill = bit_to_fill / 8;
		int32_t new_len = len + byte_to_fill + 8;

		char *p = new (nothrow) char[new_len];
		assert(p);
		char *q = p;
		memcpy(q, buf, len);
		q += len;
		for (int i = 0; i < byte_to_fill; ++i) {
			if (i == 0) {
				memcpy(q, &c1, 1);
			} else {
				memcpy(q, &c2, 1);
			}
			++q;
		}

		int64_t length = len * 8;
		memcpy(q, &length, 8);

		outbuf = p;
		outlen = new_len;
	}
	assert(bit_to_fill % 8 == 0);
}
	

inline uint32_t MD5::F(uint32_t x, uint32_t y, uint32_t z) const {
	return (x & y) | ((~x) & z);
}


inline uint32_t MD5::G(uint32_t x, uint32_t y, uint32_t z) const {
	return (x & z) | (y & (~z));
}


inline uint32_t MD5::H(uint32_t x, uint32_t y, uint32_t z) const {
	return x ^ y ^ z;
}


inline uint32_t MD5::I(uint32_t x, uint32_t y, uint32_t z) const {
	return y ^ (x | (~z));
}


#if 0
inline int32_t MD5::FF(int32_t a, int32_t b, int32_t c, int32_t d, int32_t m, int32_t s, int32_t t) {
}


inline int32_t MD5::GG(int32_t a, int32_t b, int32_t c, int32_t d, int32_t m, int32_t s, int32_t t) {
}


inline int32_t MD5::HH(int32_t a, int32_t b, int32_t c, int32_t d, int32_t m, int32_t s, int32_t t) {
}


inline int32_t MD5::II(int32_t a, int32_t b, int32_t c, int32_t d, int32_t m, int32_t s, int32_t t) {
}
#endif
