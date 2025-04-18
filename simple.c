#include "yescrypt.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char * const itoa64 =
	"./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static const uint8_t atoi64_partial[77] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
	64, 64, 64, 64, 64, 64, 64,
	12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
	25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
	64, 64, 64, 64, 64, 64,
	38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
	51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63
};

static inline uint32_t atoi64(uint8_t src)
{
	if (src >= '.' && src <= 'z')
		return atoi64_partial[src - '.'];

	return 64;
}

static uint8_t *encode64_uint32_fixed(uint8_t *dst, size_t dstlen,
    uint32_t src, uint32_t srcbits)
{
	uint32_t bits;

	for (bits = 0; bits < srcbits; bits += 6) {
		if (dstlen < 2)
			return NULL;
		*dst++ = itoa64[src & 0x3f];
		dstlen--;
		src >>= 6;
	}

	if (src || dstlen < 1)
		return NULL;

	*dst = 0;

	return dst;
}


static uint8_t *encode64(uint8_t *dst, size_t dstlen,
    const uint8_t *src, size_t srclen)
{
	size_t i;

	for (i = 0; i < srclen; ) {
		uint8_t *dnext;
		uint32_t value = 0, bits = 0;
		do {
			value |= (uint32_t)src[i++] << bits;
			bits += 8;
		} while (bits < 24 && i < srclen);
		dnext = encode64_uint32_fixed(dst, dstlen, value, bits);
		if (!dnext)
			return NULL;
		dstlen -= dnext - dst;
		dst = dnext;
	}

	if (dstlen < 1)
		return NULL;

	*dst = 0;

	return dst;
}

static const uint8_t *decode64(uint8_t *dst, size_t *dstlen,
    const uint8_t *src, size_t srclen)
{
	size_t dstpos = 0;

	while (dstpos <= *dstlen && srclen) {
		uint32_t value = 0, bits = 0;
		while (srclen--) {
			uint32_t c = atoi64(*src);
			if (c > 63) {
				srclen = 0;
				break;
			}
			src++;
			value |= c << bits;
			bits += 6;
			if (bits >= 24)
				break;
		}
		if (!bits)
			break;
		if (bits < 12) 
			goto fail;
		while (dstpos++ < *dstlen) {
			*dst++ = value;
			value >>= 8;
			bits -= 8;
			if (bits < 8) { 
				if (value)
					goto fail;
				bits = 0;
				break;
			}
		}
		if (bits)
			goto fail;
	}

	if (!srclen && dstpos <= *dstlen) {
		*dstlen = dstpos;
		return src;
	}

fail:
	*dstlen = 0;
	return NULL;
}

void yescrypt_crypt(const char *passwd, const char *saltstr) {
  yescrypt_local_t local;
  yescrypt_init_local(&local);

  yescrypt_params_t params = { 
    .p = 1,
    .flags = YESCRYPT_RW_DEFAULTS,
    .N = 4096,
    .r = 32,
  };

  const size_t saltstrlen = strlen(saltstr);

  unsigned char saltbin[64];
  size_t saltlen = sizeof(saltbin);
  decode64(saltbin, &saltlen, (uint8_t *)saltstr, saltstrlen);

  uint8_t *salt = saltbin;

  yescrypt_shared_t *shared = NULL;
  uint8_t hashbin[32];

  yescrypt_kdf(
    shared,
    &local,
    (const uint8_t *)passwd,
    strlen(passwd),
    (const uint8_t *)salt,
    saltlen,
    &params,
    hashbin,
    32
  );

  uint8_t buf[384]; // this can be 74

  uint8_t *dst = buf;

  memcpy(dst, "$y$j9T$", 7);
  dst += 7;
  memcpy(dst, saltstr, 22);
  dst += 22;
  *dst++ = '$';

  dst = encode64(dst, 44, hashbin, 32);
  *dst = 0;
  printf("%s\n", buf);
}

int main(int argc, const char* argv[]) {
  if (argc != 3) {
    printf("usage: %s (password) (salt)", argv[0]);
    return -1;
  }

  const char *passwd = argv[1];
  const char *saltstr = argv[2];
  
  yescrypt_crypt(passwd, saltstr);

  return 0;
}
