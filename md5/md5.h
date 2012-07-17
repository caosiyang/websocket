#ifndef __MD5_H__
#define __MD5_H__


#include <assert.h>
#include <math.h>
#include <iostream>
#include <string>
using namespace std;


class MD5 {
public:
	MD5();
	~MD5();
	string calc(const char *str);
	string calc(const char *buf, int32_t len);

protected:
	MD5(const MD5 &);
	MD5& operator=(const MD5 &);

private:
	uint32_t T[64];
	static const uint32_t CV[4];
	static const uint32_t R[64];

	void prepare(const char *buf, int32_t len, char *&outbuf, int32_t &outlen);
	inline uint32_t rotate_left(uint32_t num, uint32_t n) const;
	string _gen_md5_str(uint32_t a, uint32_t b, uint32_t c, uint32_t d) const;
	inline uint32_t _reverse(const uint32_t n) const;

	inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) const;
	inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) const;
	inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) const;
	inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) const;

#if 0
	inline int32_t FF(int32_t a, int32_t b, int32_t c, int32_t d, int32_t m, int32_t s, int32_t t);
	inline int32_t GG(int32_t a, int32_t b, int32_t c, int32_t d, int32_t m, int32_t s, int32_t t);
	inline int32_t HH(int32_t a, int32_t b, int32_t c, int32_t d, int32_t m, int32_t s, int32_t t);
	inline int32_t II(int32_t a, int32_t b, int32_t c, int32_t d, int32_t m, int32_t s, int32_t t);
#endif
};


#endif
