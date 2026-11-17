#define NET_SHA256_SIZE 32
#define NET_SHA512_SIZE 64
#define NET_AEAD_TAG_SIZE 16
#define NET_AEAD_NONCE_SIZE 12
#define NET_X25519_SIZE 32
#define NET_ED25519_KEY_SIZE 32
#define NET_ED25519_SIGNATURE_SIZE 64

typedef struct
{
	uint32_t state[8];
	uint64_t length;
	uint8_t block[64];
	size_t used;
} NetSha256;

typedef struct
{
	uint64_t state[8];
	uint64_t length;
	uint8_t block[128];
	size_t used;
} NetSha512;

typedef struct
{
	uint32_t r[5];
	uint32_t h[5];
	uint32_t pad[4];
} NetPoly1305;

typedef int64_t NetField[16];

static void naui__net_put_u32(uint8_t* p, uint32_t value)
{
	p[0] = (uint8_t)(value >> 24);
	p[1] = (uint8_t)(value >> 16);
	p[2] = (uint8_t)(value >> 8);
	p[3] = (uint8_t)value;
}

static uint32_t naui__net_get_u32(const uint8_t* p)
{
	return
		((uint32_t)p[0] << 24) |
		((uint32_t)p[1] << 16) |
		((uint32_t)p[2] << 8) |
		(uint32_t)p[3];
}

static void naui__net_put_u64(uint8_t* p, uint64_t value)
{
	naui__net_put_u32(p, (uint32_t)(value >> 32));
	naui__net_put_u32(p + 4, (uint32_t)value);
}

static uint64_t naui__net_get_u64(const uint8_t* p)
{
	return ((uint64_t)naui__net_get_u32(p) << 32) | (uint64_t)naui__net_get_u32(p + 4);
}

static void naui__net_wipe(void* data, size_t size)
{
	volatile uint8_t* p = (volatile uint8_t*)data;
	while (size--)
		*p++ = 0;
}

static bool naui__net_ct_equal(const void* a, const void* b, size_t size)
{
	const uint8_t* x = (const uint8_t*)a;
	const uint8_t* y = (const uint8_t*)b;
	uint8_t diff = 0;
	for (size_t i = 0; i < size; i++)
	{
		diff |= x[i] ^ y[i];
	}

	return diff == 0;
}

static bool naui__net_random_bytes(void* out, size_t size)
{
#if NAUI_WINDOWS

	typedef BOOLEAN (WINAPI* NetRtlGenRandom)(PVOID, ULONG);
	static NetRtlGenRandom generate = NULL;
	if (!generate)
	{
		HMODULE module = LoadLibraryA("advapi32.dll");
		if (module)
			generate = (NetRtlGenRandom)GetProcAddress(module, "SystemFunction036");
	}

	return generate && generate(out, (ULONG)size);

#elif defined(__linux__)

	uint8_t* p = (uint8_t*)out;
	while (size > 0)
	{
		ssize_t count = getrandom(p, size, 0);
		if (count < 0 && errno == EINTR)
			continue;

		if (count <= 0)
			return false;

		p += count;
		size -= (size_t)count;
	}

	return true;

#else

	arc4random_buf(out, size);
	return true;

#endif
}

static uint32_t naui__net_load32_le(const uint8_t* p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void naui__net_store32_le(uint8_t* p, uint32_t value)
{
	p[0] = (uint8_t)value;
	p[1] = (uint8_t)(value >> 8);
	p[2] = (uint8_t)(value >> 16);
	p[3] = (uint8_t)(value >> 24);
}

static uint32_t naui__net_rotr32(uint32_t x, int n)
{
	return (x >> n) | (x << (32 - n));
}

static uint32_t naui__net_rotl32(uint32_t x, int n)
{
	return (x << n) | (x >> (32 - n));
}

static uint64_t naui__net_rotr64(uint64_t x, int n)
{
	return (x >> n) | (x << (64 - n));
}

static const uint32_t naui__net_sha256_k[64] =
{
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static void naui__net_sha256_compress(NetSha256* sha, const uint8_t* block)
{
	uint32_t w[64];
	for (int i = 0; i < 16; i++)
	{
		w[i] = naui__net_get_u32(block + i * 4);
	}

	for (int i = 16; i < 64; i++)
	{
		uint32_t s0 = naui__net_rotr32(w[i - 15], 7) ^ naui__net_rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
		uint32_t s1 = naui__net_rotr32(w[i - 2], 17) ^ naui__net_rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	uint32_t v[8];
	memcpy(v, sha->state, sizeof(v));
	for (int i = 0; i < 64; i++)
	{
		uint32_t s1 = naui__net_rotr32(v[4], 6) ^ naui__net_rotr32(v[4], 11) ^ naui__net_rotr32(v[4], 25);
		uint32_t ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
		uint32_t t1 = v[7] + s1 + ch + naui__net_sha256_k[i] + w[i];
		uint32_t s0 = naui__net_rotr32(v[0], 2) ^ naui__net_rotr32(v[0], 13) ^ naui__net_rotr32(v[0], 22);
		uint32_t maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
		uint32_t t2 = s0 + maj;

		v[7] = v[6];
		v[6] = v[5];
		v[5] = v[4];
		v[4] = v[3] + t1;
		v[3] = v[2];
		v[2] = v[1];
		v[1] = v[0];
		v[0] = t1 + t2;
	}

	for (int i = 0; i < 8; i++)
	{
		sha->state[i] += v[i];
	}
}

static void naui__net_sha256_init(NetSha256* sha)
{
	static const uint32_t initial[8] =
	{
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
	};

	memcpy(sha->state, initial, sizeof(initial));
	sha->length = 0;
	sha->used = 0;
}

static void naui__net_sha256_update(NetSha256* sha, const void* data, size_t size)
{
	const uint8_t* p = (const uint8_t*)data;
	sha->length += size;
	while (size > 0)
	{
		size_t take = sizeof(sha->block) - sha->used;
		if (take > size)
			take = size;

		memcpy(sha->block + sha->used, p, take);
		sha->used += take;
		p += take;
		size -= take;
		if (sha->used == sizeof(sha->block))
		{
			naui__net_sha256_compress(sha, sha->block);
			sha->used = 0;
		}
	}
}

static void naui__net_sha256_final(const NetSha256* sha, uint8_t out[NET_SHA256_SIZE])
{
	NetSha256 copy = *sha;
	uint8_t tail[72] = { 0x80 };
	size_t pad = (copy.used < 56 ? 56 : 120) - copy.used;
	naui__net_put_u64(tail + pad, copy.length * 8);
	naui__net_sha256_update(&copy, tail, pad + 8);
	for (int i = 0; i < 8; i++)
	{
		naui__net_put_u32(out + i * 4, copy.state[i]);
	}
}

static void naui__net_sha256(const void* data, size_t size, uint8_t out[NET_SHA256_SIZE])
{
	NetSha256 sha;
	naui__net_sha256_init(&sha);
	naui__net_sha256_update(&sha, data, size);
	naui__net_sha256_final(&sha, out);
}

static const uint64_t naui__net_sha512_k[80] =
{
	0x428a2f98d728ae22ull, 0x7137449123ef65cdull, 0xb5c0fbcfec4d3b2full, 0xe9b5dba58189dbbcull,
	0x3956c25bf348b538ull, 0x59f111f1b605d019ull, 0x923f82a4af194f9bull, 0xab1c5ed5da6d8118ull,
	0xd807aa98a3030242ull, 0x12835b0145706fbeull, 0x243185be4ee4b28cull, 0x550c7dc3d5ffb4e2ull,
	0x72be5d74f27b896full, 0x80deb1fe3b1696b1ull, 0x9bdc06a725c71235ull, 0xc19bf174cf692694ull,
	0xe49b69c19ef14ad2ull, 0xefbe4786384f25e3ull, 0x0fc19dc68b8cd5b5ull, 0x240ca1cc77ac9c65ull,
	0x2de92c6f592b0275ull, 0x4a7484aa6ea6e483ull, 0x5cb0a9dcbd41fbd4ull, 0x76f988da831153b5ull,
	0x983e5152ee66dfabull, 0xa831c66d2db43210ull, 0xb00327c898fb213full, 0xbf597fc7beef0ee4ull,
	0xc6e00bf33da88fc2ull, 0xd5a79147930aa725ull, 0x06ca6351e003826full, 0x142929670a0e6e70ull,
	0x27b70a8546d22ffcull, 0x2e1b21385c26c926ull, 0x4d2c6dfc5ac42aedull, 0x53380d139d95b3dfull,
	0x650a73548baf63deull, 0x766a0abb3c77b2a8ull, 0x81c2c92e47edaee6ull, 0x92722c851482353bull,
	0xa2bfe8a14cf10364ull, 0xa81a664bbc423001ull, 0xc24b8b70d0f89791ull, 0xc76c51a30654be30ull,
	0xd192e819d6ef5218ull, 0xd69906245565a910ull, 0xf40e35855771202aull, 0x106aa07032bbd1b8ull,
	0x19a4c116b8d2d0c8ull, 0x1e376c085141ab53ull, 0x2748774cdf8eeb99ull, 0x34b0bcb5e19b48a8ull,
	0x391c0cb3c5c95a63ull, 0x4ed8aa4ae3418acbull, 0x5b9cca4f7763e373ull, 0x682e6ff3d6b2b8a3ull,
	0x748f82ee5defb2fcull, 0x78a5636f43172f60ull, 0x84c87814a1f0ab72ull, 0x8cc702081a6439ecull,
	0x90befffa23631e28ull, 0xa4506cebde82bde9ull, 0xbef9a3f7b2c67915ull, 0xc67178f2e372532bull,
	0xca273eceea26619cull, 0xd186b8c721c0c207ull, 0xeada7dd6cde0eb1eull, 0xf57d4f7fee6ed178ull,
	0x06f067aa72176fbaull, 0x0a637dc5a2c898a6ull, 0x113f9804bef90daeull, 0x1b710b35131c471bull,
	0x28db77f523047d84ull, 0x32caab7b40c72493ull, 0x3c9ebe0a15c9bebcull, 0x431d67c49c100d4cull,
	0x4cc5d4becb3e42b6ull, 0x597f299cfc657e2aull, 0x5fcb6fab3ad6faecull, 0x6c44198c4a475817ull,
};

static void naui__net_sha512_compress(NetSha512* sha, const uint8_t* block)
{
	uint64_t w[80];
	for (int i = 0; i < 16; i++)
	{
		w[i] = naui__net_get_u64(block + i * 8);
	}

	for (int i = 16; i < 80; i++)
	{
		uint64_t s0 = naui__net_rotr64(w[i - 15], 1) ^ naui__net_rotr64(w[i - 15], 8) ^ (w[i - 15] >> 7);
		uint64_t s1 = naui__net_rotr64(w[i - 2], 19) ^ naui__net_rotr64(w[i - 2], 61) ^ (w[i - 2] >> 6);
		w[i] = w[i - 16] + s0 + w[i - 7] + s1;
	}

	uint64_t v[8];
	memcpy(v, sha->state, sizeof(v));
	for (int i = 0; i < 80; i++)
	{
		uint64_t s1 = naui__net_rotr64(v[4], 14) ^ naui__net_rotr64(v[4], 18) ^ naui__net_rotr64(v[4], 41);
		uint64_t ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
		uint64_t t1 = v[7] + s1 + ch + naui__net_sha512_k[i] + w[i];
		uint64_t s0 = naui__net_rotr64(v[0], 28) ^ naui__net_rotr64(v[0], 34) ^ naui__net_rotr64(v[0], 39);
		uint64_t maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
		uint64_t t2 = s0 + maj;

		v[7] = v[6];
		v[6] = v[5];
		v[5] = v[4];
		v[4] = v[3] + t1;
		v[3] = v[2];
		v[2] = v[1];
		v[1] = v[0];
		v[0] = t1 + t2;
	}

	for (int i = 0; i < 8; i++)
	{
		sha->state[i] += v[i];
	}
}

static void naui__net_sha512_init(NetSha512* sha)
{
	static const uint64_t initial[8] =
	{
		0x6a09e667f3bcc908ull, 0xbb67ae8584caa73bull, 0x3c6ef372fe94f82bull, 0xa54ff53a5f1d36f1ull,
		0x510e527fade682d1ull, 0x9b05688c2b3e6c1full, 0x1f83d9abfb41bd6bull, 0x5be0cd19137e2179ull,
	};

	memcpy(sha->state, initial, sizeof(initial));
	sha->length = 0;
	sha->used = 0;
}

static void naui__net_sha512_update(NetSha512* sha, const void* data, size_t size)
{
	const uint8_t* p = (const uint8_t*)data;
	sha->length += size;
	while (size > 0)
	{
		size_t take = sizeof(sha->block) - sha->used;
		if (take > size)
			take = size;

		memcpy(sha->block + sha->used, p, take);
		sha->used += take;
		p += take;
		size -= take;
		if (sha->used == sizeof(sha->block))
		{
			naui__net_sha512_compress(sha, sha->block);
			sha->used = 0;
		}
	}
}

static void naui__net_sha512_final(NetSha512* sha, uint8_t out[NET_SHA512_SIZE])
{
	uint8_t tail[144] = { 0x80 };
	size_t pad = (sha->used < 112 ? 112 : 240) - sha->used;
	uint64_t bits = sha->length * 8;
	naui__net_put_u64(tail + pad + 8, bits);
	naui__net_sha512_update(sha, tail, pad + 16);
	for (int i = 0; i < 8; i++)
	{
		naui__net_put_u64(out + i * 8, sha->state[i]);
	}
}

static void naui__net_hmac_sha256(const uint8_t* key, size_t key_size, const void* data, size_t size, uint8_t out[NET_SHA256_SIZE])
{
	uint8_t block[64] = { 0 };
	if (key_size > sizeof(block))
		naui__net_sha256(key, key_size, block);
	else
		memcpy(block, key, key_size);

	uint8_t pad[64];
	for (size_t i = 0; i < sizeof(pad); i++)
	{
		pad[i] = block[i] ^ 0x36;
	}

	uint8_t inner[NET_SHA256_SIZE];
	NetSha256 sha;
	naui__net_sha256_init(&sha);
	naui__net_sha256_update(&sha, pad, sizeof(pad));
	naui__net_sha256_update(&sha, data, size);
	naui__net_sha256_final(&sha, inner);

	for (size_t i = 0; i < sizeof(pad); i++)
	{
		pad[i] = block[i] ^ 0x5c;
	}

	naui__net_sha256_init(&sha);
	naui__net_sha256_update(&sha, pad, sizeof(pad));
	naui__net_sha256_update(&sha, inner, sizeof(inner));
	naui__net_sha256_final(&sha, out);

	naui__net_wipe(block, sizeof(block));
	naui__net_wipe(pad, sizeof(pad));
}

static void naui__net_hkdf_extract(const uint8_t* salt, size_t salt_size, const uint8_t* ikm, size_t ikm_size, uint8_t out[NET_SHA256_SIZE])
{
	naui__net_hmac_sha256(salt, salt_size, ikm, ikm_size, out);
}

static void naui__net_hkdf_expand_label(const uint8_t secret[NET_SHA256_SIZE], const char* label, const uint8_t* context, size_t context_size, uint8_t* out, size_t out_size)
{
	uint8_t info[128];
	size_t label_size = strlen(label);
	size_t n = 0;

	info[n++] = (uint8_t)(out_size >> 8);
	info[n++] = (uint8_t)out_size;
	info[n++] = (uint8_t)(6 + label_size);
	memcpy(info + n, "tls13 ", 6);
	n += 6;
	memcpy(info + n, label, label_size);
	n += label_size;
	info[n++] = (uint8_t)context_size;
	if (context_size)
		memcpy(info + n, context, context_size);

	n += context_size;
	info[n++] = 1;

	uint8_t block[NET_SHA256_SIZE];
	naui__net_hmac_sha256(secret, NET_SHA256_SIZE, info, n, block);
	memcpy(out, block, out_size);
	naui__net_wipe(block, sizeof(block));
}

#define NET_CHACHA_QUARTER(a, b, c, d) \
	a += b; d = naui__net_rotl32(d ^ a, 16); \
	c += d; b = naui__net_rotl32(b ^ c, 12); \
	a += b; d = naui__net_rotl32(d ^ a, 8); \
	c += d; b = naui__net_rotl32(b ^ c, 7)

static void naui__net_chacha20_block(const uint8_t key[32], uint32_t counter, const uint8_t nonce[12], uint8_t out[64])
{
	uint32_t s[16] = { 0x61707865, 0x3320646e, 0x79622d32, 0x6b206574 };
	for (int i = 0; i < 8; i++)
	{
		s[4 + i] = naui__net_load32_le(key + i * 4);
	}

	s[12] = counter;
	s[13] = naui__net_load32_le(nonce);
	s[14] = naui__net_load32_le(nonce + 4);
	s[15] = naui__net_load32_le(nonce + 8);

	uint32_t x[16];
	memcpy(x, s, sizeof(x));
	for (int i = 0; i < 10; i++)
	{
		NET_CHACHA_QUARTER(x[0], x[4], x[8], x[12]);
		NET_CHACHA_QUARTER(x[1], x[5], x[9], x[13]);
		NET_CHACHA_QUARTER(x[2], x[6], x[10], x[14]);
		NET_CHACHA_QUARTER(x[3], x[7], x[11], x[15]);
		NET_CHACHA_QUARTER(x[0], x[5], x[10], x[15]);
		NET_CHACHA_QUARTER(x[1], x[6], x[11], x[12]);
		NET_CHACHA_QUARTER(x[2], x[7], x[8], x[13]);
		NET_CHACHA_QUARTER(x[3], x[4], x[9], x[14]);
	}

	for (int i = 0; i < 16; i++)
	{
		naui__net_store32_le(out + i * 4, x[i] + s[i]);
	}
}

static void naui__net_chacha20_xor(const uint8_t key[32], uint32_t counter, const uint8_t nonce[12], const uint8_t* in, uint8_t* out, size_t size)
{
	uint8_t block[64];
	while (size > 0)
	{
		naui__net_chacha20_block(key, counter++, nonce, block);
		size_t take = size < sizeof(block) ? size : sizeof(block);
		for (size_t i = 0; i < take; i++)
		{
			out[i] = in[i] ^ block[i];
		}

		in += take;
		out += take;
		size -= take;
	}

	naui__net_wipe(block, sizeof(block));
}

static void naui__net_poly1305_init(NetPoly1305* poly, const uint8_t key[32])
{
	poly->r[0] = naui__net_load32_le(key) & 0x3ffffff;
	poly->r[1] = (naui__net_load32_le(key + 3) >> 2) & 0x3ffff03;
	poly->r[2] = (naui__net_load32_le(key + 6) >> 4) & 0x3ffc0ff;
	poly->r[3] = (naui__net_load32_le(key + 9) >> 6) & 0x3f03fff;
	poly->r[4] = (naui__net_load32_le(key + 12) >> 8) & 0x00fffff;
	memset(poly->h, 0, sizeof(poly->h));
	for (int i = 0; i < 4; i++)
	{
		poly->pad[i] = naui__net_load32_le(key + 16 + i * 4);
	}
}

static void naui__net_poly1305_block(NetPoly1305* poly, const uint8_t block[16])
{
	uint32_t r0 = poly->r[0], r1 = poly->r[1], r2 = poly->r[2], r3 = poly->r[3], r4 = poly->r[4];
	uint32_t s1 = r1 * 5, s2 = r2 * 5, s3 = r3 * 5, s4 = r4 * 5;

	uint32_t h0 = poly->h[0] + (naui__net_load32_le(block) & 0x3ffffff);
	uint32_t h1 = poly->h[1] + ((naui__net_load32_le(block + 3) >> 2) & 0x3ffffff);
	uint32_t h2 = poly->h[2] + ((naui__net_load32_le(block + 6) >> 4) & 0x3ffffff);
	uint32_t h3 = poly->h[3] + ((naui__net_load32_le(block + 9) >> 6) & 0x3ffffff);
	uint32_t h4 = poly->h[4] + ((naui__net_load32_le(block + 12) >> 8) | (1u << 24));

	uint64_t d0 = (uint64_t)h0 * r0 + (uint64_t)h1 * s4 + (uint64_t)h2 * s3 + (uint64_t)h3 * s2 + (uint64_t)h4 * s1;
	uint64_t d1 = (uint64_t)h0 * r1 + (uint64_t)h1 * r0 + (uint64_t)h2 * s4 + (uint64_t)h3 * s3 + (uint64_t)h4 * s2;
	uint64_t d2 = (uint64_t)h0 * r2 + (uint64_t)h1 * r1 + (uint64_t)h2 * r0 + (uint64_t)h3 * s4 + (uint64_t)h4 * s3;
	uint64_t d3 = (uint64_t)h0 * r3 + (uint64_t)h1 * r2 + (uint64_t)h2 * r1 + (uint64_t)h3 * r0 + (uint64_t)h4 * s4;
	uint64_t d4 = (uint64_t)h0 * r4 + (uint64_t)h1 * r3 + (uint64_t)h2 * r2 + (uint64_t)h3 * r1 + (uint64_t)h4 * r0;

	d1 += d0 >> 26;
	d2 += d1 >> 26;
	d3 += d2 >> 26;
	d4 += d3 >> 26;
	h0 = (uint32_t)d0 & 0x3ffffff;
	h1 = (uint32_t)d1 & 0x3ffffff;
	h2 = (uint32_t)d2 & 0x3ffffff;
	h3 = (uint32_t)d3 & 0x3ffffff;
	h4 = (uint32_t)d4 & 0x3ffffff;
	h0 += (uint32_t)(d4 >> 26) * 5;
	h1 += h0 >> 26;
	h0 &= 0x3ffffff;

	poly->h[0] = h0;
	poly->h[1] = h1;
	poly->h[2] = h2;
	poly->h[3] = h3;
	poly->h[4] = h4;
}

static void naui__net_poly1305_padded(NetPoly1305* poly, const uint8_t* data, size_t size)
{
	while (size >= 16)
	{
		naui__net_poly1305_block(poly, data);
		data += 16;
		size -= 16;
	}

	if (size > 0)
	{
		uint8_t block[16] = { 0 };
		memcpy(block, data, size);
		naui__net_poly1305_block(poly, block);
	}
}

static void naui__net_poly1305_final(NetPoly1305* poly, uint8_t tag[16])
{
	uint32_t h0 = poly->h[0], h1 = poly->h[1], h2 = poly->h[2], h3 = poly->h[3], h4 = poly->h[4];

	h2 += h1 >> 26;
	h1 &= 0x3ffffff;
	h3 += h2 >> 26;
	h2 &= 0x3ffffff;
	h4 += h3 >> 26;
	h3 &= 0x3ffffff;
	h0 += (h4 >> 26) * 5;
	h4 &= 0x3ffffff;
	h1 += h0 >> 26;
	h0 &= 0x3ffffff;

	uint32_t g0 = h0 + 5;
	uint32_t g1 = h1 + (g0 >> 26);
	g0 &= 0x3ffffff;
	uint32_t g2 = h2 + (g1 >> 26);
	g1 &= 0x3ffffff;
	uint32_t g3 = h3 + (g2 >> 26);
	g2 &= 0x3ffffff;
	uint32_t g4 = h4 + (g3 >> 26) - (1u << 26);
	g3 &= 0x3ffffff;

	uint32_t take_g = (g4 >> 31) - 1;
	uint32_t keep_h = ~take_g;
	h0 = (h0 & keep_h) | (g0 & take_g);
	h1 = (h1 & keep_h) | (g1 & take_g);
	h2 = (h2 & keep_h) | (g2 & take_g);
	h3 = (h3 & keep_h) | (g3 & take_g);
	h4 = (h4 & keep_h) | (g4 & take_g);

	h0 = h0 | (h1 << 26);
	h1 = (h1 >> 6) | (h2 << 20);
	h2 = (h2 >> 12) | (h3 << 14);
	h3 = (h3 >> 18) | (h4 << 8);

	uint64_t f = (uint64_t)h0 + poly->pad[0];
	naui__net_store32_le(tag, (uint32_t)f);
	f = (uint64_t)h1 + poly->pad[1] + (f >> 32);
	naui__net_store32_le(tag + 4, (uint32_t)f);
	f = (uint64_t)h2 + poly->pad[2] + (f >> 32);
	naui__net_store32_le(tag + 8, (uint32_t)f);
	f = (uint64_t)h3 + poly->pad[3] + (f >> 32);
	naui__net_store32_le(tag + 12, (uint32_t)f);

	naui__net_wipe(poly, sizeof(*poly));
}

static void naui__net_chacha20_poly1305_tag(const uint8_t key[32], const uint8_t nonce[12], const uint8_t* aad, size_t aad_size, const uint8_t* cipher, size_t size, uint8_t tag[16])
{
	uint8_t block[64];
	naui__net_chacha20_block(key, 0, nonce, block);

	NetPoly1305 poly;
	naui__net_poly1305_init(&poly, block);
	naui__net_poly1305_padded(&poly, aad, aad_size);
	naui__net_poly1305_padded(&poly, cipher, size);

	uint8_t lengths[16] = { 0 };
	naui__net_store32_le(lengths, (uint32_t)aad_size);
	naui__net_store32_le(lengths + 8, (uint32_t)size);
	naui__net_poly1305_block(&poly, lengths);
	naui__net_poly1305_final(&poly, tag);
	naui__net_wipe(block, sizeof(block));
}

static const uint8_t naui__net_aes_sbox[256] =
{
	0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
	0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
	0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
	0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
	0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
	0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
	0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
	0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
	0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
	0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
	0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
	0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
	0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
	0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
	0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
	0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16,
};

static uint8_t naui__net_aes_double(uint8_t x)
{
	return (uint8_t)((x << 1) ^ ((x >> 7) * 0x1b));
}

static void naui__net_aes128_encrypt(const uint8_t key[16], const uint8_t in[16], uint8_t out[16])
{
	uint8_t round_keys[176];
	memcpy(round_keys, key, 16);

	uint8_t rcon = 1;
	for (int i = 16; i < 176; i += 4)
	{
		uint8_t t[4];
		memcpy(t, round_keys + i - 4, 4);
		if (i % 16 == 0)
		{
			uint8_t first = t[0];
			t[0] = naui__net_aes_sbox[t[1]] ^ rcon;
			t[1] = naui__net_aes_sbox[t[2]];
			t[2] = naui__net_aes_sbox[t[3]];
			t[3] = naui__net_aes_sbox[first];
			rcon = naui__net_aes_double(rcon);
		}

		for (int j = 0; j < 4; j++)
		{
			round_keys[i + j] = round_keys[i + j - 16] ^ t[j];
		}
	}

	uint8_t s[16];
	for (int i = 0; i < 16; i++)
	{
		s[i] = in[i] ^ round_keys[i];
	}

	for (int round = 1; round <= 10; round++)
	{
		uint8_t t[16];
		for (int i = 0; i < 16; i++)
		{
			t[i] = naui__net_aes_sbox[s[(i + (i % 4) * 4) % 16]];
		}

		if (round < 10)
		{
			for (int c = 0; c < 16; c += 4)
			{
				uint8_t all = t[c] ^ t[c + 1] ^ t[c + 2] ^ t[c + 3];
				uint8_t first = t[c];
				s[c] = t[c] ^ all ^ naui__net_aes_double(t[c] ^ t[c + 1]);
				s[c + 1] = t[c + 1] ^ all ^ naui__net_aes_double(t[c + 1] ^ t[c + 2]);
				s[c + 2] = t[c + 2] ^ all ^ naui__net_aes_double(t[c + 2] ^ t[c + 3]);
				s[c + 3] = t[c + 3] ^ all ^ naui__net_aes_double(t[c + 3] ^ first);
			}
		}
		else
		{
			memcpy(s, t, sizeof(s));
		}

		for (int i = 0; i < 16; i++)
		{
			s[i] ^= round_keys[round * 16 + i];
		}
	}

	memcpy(out, s, 16);
}

static void naui__net_ghash_multiply(uint8_t x[16], const uint8_t h[16])
{
	uint8_t z[16] = { 0 };
	uint8_t v[16];
	memcpy(v, h, 16);

	for (int i = 0; i < 128; i++)
	{
		if (x[i / 8] & (0x80 >> (i % 8)))
		{
			for (int j = 0; j < 16; j++)
			{
				z[j] ^= v[j];
			}
		}

		uint8_t carry = v[15] & 1;
		for (int j = 15; j > 0; j--)
		{
			v[j] = (uint8_t)((v[j] >> 1) | (v[j - 1] << 7));
		}

		v[0] >>= 1;
		if (carry)
			v[0] ^= 0xe1;
	}

	memcpy(x, z, 16);
}

static void naui__net_ghash_update(uint8_t y[16], const uint8_t h[16], const uint8_t* data, size_t size)
{
	while (size > 0)
	{
		size_t take = size < 16 ? size : 16;
		for (size_t i = 0; i < take; i++)
		{
			y[i] ^= data[i];
		}

		naui__net_ghash_multiply(y, h);
		data += take;
		size -= take;
	}
}

static void naui__net_aes128_gcm_xor(const uint8_t key[16], const uint8_t nonce[12], const uint8_t* in, uint8_t* out, size_t size)
{
	uint8_t counter[16];
	uint8_t block[16];
	memcpy(counter, nonce, 12);

	uint32_t index = 2;
	while (size > 0)
	{
		naui__net_put_u32(counter + 12, index++);
		naui__net_aes128_encrypt(key, counter, block);
		size_t take = size < 16 ? size : 16;
		for (size_t i = 0; i < take; i++)
		{
			out[i] = in[i] ^ block[i];
		}

		in += take;
		out += take;
		size -= take;
	}
}

static void naui__net_aes128_gcm_tag(const uint8_t key[16], const uint8_t nonce[12], const uint8_t* aad, size_t aad_size, const uint8_t* cipher, size_t size, uint8_t tag[16])
{
	uint8_t zero[16] = { 0 };
	uint8_t h[16];
	naui__net_aes128_encrypt(key, zero, h);

	uint8_t y[16] = { 0 };
	naui__net_ghash_update(y, h, aad, aad_size);
	naui__net_ghash_update(y, h, cipher, size);

	uint8_t lengths[16];
	naui__net_put_u64(lengths, (uint64_t)aad_size * 8);
	naui__net_put_u64(lengths + 8, (uint64_t)size * 8);
	naui__net_ghash_update(y, h, lengths, sizeof(lengths));

	uint8_t counter[16];
	memcpy(counter, nonce, 12);
	naui__net_put_u32(counter + 12, 1);
	naui__net_aes128_encrypt(key, counter, tag);
	for (int i = 0; i < 16; i++)
	{
		tag[i] ^= y[i];
	}
}

static void naui__net_aead_seal(bool aes, const uint8_t* key, const uint8_t nonce[12], const uint8_t* aad, size_t aad_size, const uint8_t* in, size_t size, uint8_t* out)
{
	if (aes)
	{
		naui__net_aes128_gcm_xor(key, nonce, in, out, size);
		naui__net_aes128_gcm_tag(key, nonce, aad, aad_size, out, size, out + size);
		return;
	}

	naui__net_chacha20_xor(key, 1, nonce, in, out, size);
	naui__net_chacha20_poly1305_tag(key, nonce, aad, aad_size, out, size, out + size);
}

static bool naui__net_aead_open(bool aes, const uint8_t* key, const uint8_t nonce[12], const uint8_t* aad, size_t aad_size, const uint8_t* in, size_t size, uint8_t* out)
{
	if (size < NET_AEAD_TAG_SIZE)
		return false;

	size -= NET_AEAD_TAG_SIZE;
	uint8_t tag[NET_AEAD_TAG_SIZE];
	if (aes)
		naui__net_aes128_gcm_tag(key, nonce, aad, aad_size, in, size, tag);
	else
		naui__net_chacha20_poly1305_tag(key, nonce, aad, aad_size, in, size, tag);

	if (!naui__net_ct_equal(tag, in + size, NET_AEAD_TAG_SIZE))
		return false;

	if (aes)
		naui__net_aes128_gcm_xor(key, nonce, in, out, size);
	else
		naui__net_chacha20_xor(key, 1, nonce, in, out, size);

	return true;
}

static const NetField naui__net_field_zero = { 0 };
static const NetField naui__net_field_one = { 1 };
static const NetField naui__net_field_121665 = { 0xdb41, 1 };

static const NetField naui__net_ed_d =
{
	0x78a3, 0x1359, 0x4dca, 0x75eb, 0xd8ab, 0x4141, 0x0a4d, 0x0070,
	0xe898, 0x7779, 0x4079, 0x8cc7, 0xfe73, 0x2b6f, 0x6cee, 0x5203,
};

static const NetField naui__net_ed_d2 =
{
	0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283, 0x149a, 0x00e0,
	0xd130, 0xeef3, 0x80f2, 0x198e, 0xfce7, 0x56df, 0xd9dc, 0x2406,
};

static const NetField naui__net_ed_x =
{
	0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525, 0xc760, 0x692c,
	0xdc5c, 0xfdd6, 0xe231, 0xc0a4, 0x53fe, 0xcd6e, 0x36d3, 0x2169,
};

static const NetField naui__net_ed_y =
{
	0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
	0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
};

static const NetField naui__net_ed_sqrt_m1 =
{
	0xa0b0, 0x4a0e, 0x1b27, 0xc4ee, 0xe478, 0xad2f, 0x1806, 0x2f43,
	0xd7a7, 0x3dfb, 0x0099, 0x2b4d, 0xdf0b, 0x4fc1, 0x2480, 0x2b83,
};

static const uint8_t naui__net_ed_order[32] =
{
	0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
};

static void naui__net_field_copy(NetField out, const NetField in)
{
	memcpy(out, in, sizeof(NetField));
}

static void naui__net_field_carry(NetField o)
{
	for (int i = 0; i < 16; i++)
	{
		o[i] += (int64_t)1 << 16;
		int64_t c = o[i] >> 16;
		o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
		o[i] -= c * 65536;
	}
}

static void naui__net_field_select(NetField p, NetField q, int swap)
{
	int64_t mask = ~((int64_t)swap - 1);
	for (int i = 0; i < 16; i++)
	{
		int64_t t = mask & (p[i] ^ q[i]);
		p[i] ^= t;
		q[i] ^= t;
	}
}

static void naui__net_field_pack(uint8_t out[32], const NetField in)
{
	NetField m;
	NetField t;
	naui__net_field_copy(t, in);
	naui__net_field_carry(t);
	naui__net_field_carry(t);
	naui__net_field_carry(t);

	for (int j = 0; j < 2; j++)
	{
		m[0] = t[0] - 0xffed;
		for (int i = 1; i < 15; i++)
		{
			m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
			m[i - 1] &= 0xffff;
		}

		m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
		int borrow = (int)((m[15] >> 16) & 1);
		m[14] &= 0xffff;
		naui__net_field_select(t, m, 1 - borrow);
	}

	for (int i = 0; i < 16; i++)
	{
		out[2 * i] = (uint8_t)(t[i] & 0xff);
		out[2 * i + 1] = (uint8_t)(t[i] >> 8);
	}
}

static void naui__net_field_unpack(NetField out, const uint8_t in[32])
{
	for (int i = 0; i < 16; i++)
	{
		out[i] = in[2 * i] + ((int64_t)in[2 * i + 1] << 8);
	}

	out[15] &= 0x7fff;
}

static void naui__net_field_add(NetField o, const NetField a, const NetField b)
{
	for (int i = 0; i < 16; i++)
	{
		o[i] = a[i] + b[i];
	}
}

static void naui__net_field_sub(NetField o, const NetField a, const NetField b)
{
	for (int i = 0; i < 16; i++)
	{
		o[i] = a[i] - b[i];
	}
}

static void naui__net_field_mul(NetField o, const NetField a, const NetField b)
{
	int64_t t[31] = { 0 };
	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			t[i + j] += a[i] * b[j];
		}
	}

	for (int i = 0; i < 15; i++)
	{
		t[i] += 38 * t[i + 16];
	}

	memcpy(o, t, sizeof(NetField));
	naui__net_field_carry(o);
	naui__net_field_carry(o);
}

static void naui__net_field_square(NetField o, const NetField a)
{
	naui__net_field_mul(o, a, a);
}

static void naui__net_field_invert(NetField o, const NetField in)
{
	NetField c;
	naui__net_field_copy(c, in);
	for (int a = 253; a >= 0; a--)
	{
		naui__net_field_square(c, c);
		if (a != 2 && a != 4)
			naui__net_field_mul(c, c, in);
	}

	naui__net_field_copy(o, c);
}

static void naui__net_field_pow2523(NetField o, const NetField in)
{
	NetField c;
	naui__net_field_copy(c, in);
	for (int a = 250; a >= 0; a--)
	{
		naui__net_field_square(c, c);
		if (a != 1)
			naui__net_field_mul(c, c, in);
	}

	naui__net_field_copy(o, c);
}

static bool naui__net_field_equal(const NetField a, const NetField b)
{
	uint8_t c[32];
	uint8_t d[32];
	naui__net_field_pack(c, a);
	naui__net_field_pack(d, b);
	return naui__net_ct_equal(c, d, 32);
}

static uint8_t naui__net_field_parity(const NetField a)
{
	uint8_t d[32];
	naui__net_field_pack(d, a);
	return d[0] & 1;
}

static void naui__net_x25519(uint8_t out[32], const uint8_t scalar[32], const uint8_t point[32])
{
	uint8_t z[32];
	NetField x, a, b, c, d, e, f;

	memcpy(z, scalar, 32);
	z[31] = (z[31] & 127) | 64;
	z[0] &= 248;

	naui__net_field_unpack(x, point);
	naui__net_field_copy(b, x);
	naui__net_field_copy(a, naui__net_field_one);
	naui__net_field_copy(d, naui__net_field_one);
	naui__net_field_copy(c, naui__net_field_zero);

	for (int i = 254; i >= 0; i--)
	{
		int bit = (z[i >> 3] >> (i & 7)) & 1;
		naui__net_field_select(a, b, bit);
		naui__net_field_select(c, d, bit);
		naui__net_field_add(e, a, c);
		naui__net_field_sub(a, a, c);
		naui__net_field_add(c, b, d);
		naui__net_field_sub(b, b, d);
		naui__net_field_square(d, e);
		naui__net_field_square(f, a);
		naui__net_field_mul(a, c, a);
		naui__net_field_mul(c, b, e);
		naui__net_field_add(e, a, c);
		naui__net_field_sub(a, a, c);
		naui__net_field_square(b, a);
		naui__net_field_sub(c, d, f);
		naui__net_field_mul(a, c, naui__net_field_121665);
		naui__net_field_add(a, a, d);
		naui__net_field_mul(c, c, a);
		naui__net_field_mul(a, d, f);
		naui__net_field_mul(d, b, x);
		naui__net_field_square(b, e);
		naui__net_field_select(a, b, bit);
		naui__net_field_select(c, d, bit);
	}

	naui__net_field_invert(c, c);
	naui__net_field_mul(a, a, c);
	naui__net_field_pack(out, a);
	naui__net_wipe(z, sizeof(z));
}

static void naui__net_x25519_public(uint8_t out[32], const uint8_t scalar[32])
{
	static const uint8_t base[32] = { 9 };
	naui__net_x25519(out, scalar, base);
}

static void naui__net_ed_add(NetField p[4], NetField q[4])
{
	NetField a, b, c, d, t, e, f, g, h;

	naui__net_field_sub(a, p[1], p[0]);
	naui__net_field_sub(t, q[1], q[0]);
	naui__net_field_mul(a, a, t);
	naui__net_field_add(b, p[0], p[1]);
	naui__net_field_add(t, q[0], q[1]);
	naui__net_field_mul(b, b, t);
	naui__net_field_mul(c, p[3], q[3]);
	naui__net_field_mul(c, c, naui__net_ed_d2);
	naui__net_field_mul(d, p[2], q[2]);
	naui__net_field_add(d, d, d);
	naui__net_field_sub(e, b, a);
	naui__net_field_sub(f, d, c);
	naui__net_field_add(g, d, c);
	naui__net_field_add(h, b, a);

	naui__net_field_mul(p[0], e, f);
	naui__net_field_mul(p[1], h, g);
	naui__net_field_mul(p[2], g, f);
	naui__net_field_mul(p[3], e, h);
}

static void naui__net_ed_swap(NetField p[4], NetField q[4], int swap)
{
	for (int i = 0; i < 4; i++)
	{
		naui__net_field_select(p[i], q[i], swap);
	}
}

static void naui__net_ed_pack(uint8_t out[32], NetField p[4])
{
	NetField tx, ty, zi;
	naui__net_field_invert(zi, p[2]);
	naui__net_field_mul(tx, p[0], zi);
	naui__net_field_mul(ty, p[1], zi);
	naui__net_field_pack(out, ty);
	out[31] ^= naui__net_field_parity(tx) << 7;
}

static void naui__net_ed_scalar_mul(NetField p[4], NetField q[4], const uint8_t scalar[32])
{
	naui__net_field_copy(p[0], naui__net_field_zero);
	naui__net_field_copy(p[1], naui__net_field_one);
	naui__net_field_copy(p[2], naui__net_field_one);
	naui__net_field_copy(p[3], naui__net_field_zero);

	for (int i = 255; i >= 0; i--)
	{
		int bit = (scalar[i / 8] >> (i & 7)) & 1;
		naui__net_ed_swap(p, q, bit);
		naui__net_ed_add(q, p);
		naui__net_ed_add(p, p);
		naui__net_ed_swap(p, q, bit);
	}
}

static void naui__net_ed_scalar_base(NetField p[4], const uint8_t scalar[32])
{
	NetField q[4];
	naui__net_field_copy(q[0], naui__net_ed_x);
	naui__net_field_copy(q[1], naui__net_ed_y);
	naui__net_field_copy(q[2], naui__net_field_one);
	naui__net_field_mul(q[3], naui__net_ed_x, naui__net_ed_y);
	naui__net_ed_scalar_mul(p, q, scalar);
}

static void naui__net_ed_reduce_wide(uint8_t out[32], int64_t x[64])
{
	for (int i = 63; i >= 32; i--)
	{
		int64_t carry = 0;
		int j;
		for (j = i - 32; j < i - 12; j++)
		{
			x[j] += carry - 16 * x[i] * naui__net_ed_order[j - (i - 32)];
			carry = (x[j] + 128) >> 8;
			x[j] -= carry * 256;
		}

		x[j] += carry;
		x[i] = 0;
	}

	int64_t carry = 0;
	for (int j = 0; j < 32; j++)
	{
		x[j] += carry - (x[31] >> 4) * naui__net_ed_order[j];
		carry = x[j] >> 8;
		x[j] &= 255;
	}

	for (int j = 0; j < 32; j++)
	{
		x[j] -= carry * naui__net_ed_order[j];
	}

	for (int i = 0; i < 32; i++)
	{
		x[i + 1] += x[i] >> 8;
		out[i] = (uint8_t)(x[i] & 255);
	}
}

static void naui__net_ed_reduce(uint8_t hash[64])
{
	int64_t x[64];
	for (int i = 0; i < 64; i++)
	{
		x[i] = hash[i];
	}

	memset(hash, 0, 64);
	naui__net_ed_reduce_wide(hash, x);
}

static void naui__net_ed_expand_seed(const uint8_t seed[32], uint8_t expanded[64])
{
	NetSha512 sha;
	naui__net_sha512_init(&sha);
	naui__net_sha512_update(&sha, seed, 32);
	naui__net_sha512_final(&sha, expanded);
	expanded[0] &= 248;
	expanded[31] &= 127;
	expanded[31] |= 64;
}

static void naui__net_ed25519_public(const uint8_t seed[32], uint8_t public_key[32])
{
	uint8_t expanded[64];
	NetField p[4];
	naui__net_ed_expand_seed(seed, expanded);
	naui__net_ed_scalar_base(p, expanded);
	naui__net_ed_pack(public_key, p);
	naui__net_wipe(expanded, sizeof(expanded));
}

static void naui__net_ed25519_sign(const uint8_t seed[32], const uint8_t public_key[32], const uint8_t* message, size_t size, uint8_t signature[64])
{
	uint8_t expanded[64];
	uint8_t r[64];
	uint8_t h[64];
	NetField p[4];
	NetSha512 sha;

	naui__net_ed_expand_seed(seed, expanded);

	naui__net_sha512_init(&sha);
	naui__net_sha512_update(&sha, expanded + 32, 32);
	naui__net_sha512_update(&sha, message, size);
	naui__net_sha512_final(&sha, r);
	naui__net_ed_reduce(r);
	naui__net_ed_scalar_base(p, r);
	naui__net_ed_pack(signature, p);

	naui__net_sha512_init(&sha);
	naui__net_sha512_update(&sha, signature, 32);
	naui__net_sha512_update(&sha, public_key, 32);
	naui__net_sha512_update(&sha, message, size);
	naui__net_sha512_final(&sha, h);
	naui__net_ed_reduce(h);

	int64_t x[64] = { 0 };
	for (int i = 0; i < 32; i++)
	{
		x[i] = r[i];
	}

	for (int i = 0; i < 32; i++)
	{
		for (int j = 0; j < 32; j++)
		{
			x[i + j] += (int64_t)h[i] * expanded[j];
		}
	}

	naui__net_ed_reduce_wide(signature + 32, x);
	naui__net_wipe(expanded, sizeof(expanded));
	naui__net_wipe(r, sizeof(r));
	naui__net_wipe(x, sizeof(x));
}

static bool naui__net_ed_unpack_negated(NetField r[4], const uint8_t p[32])
{
	NetField t, chk, num, den, den2, den4, den6;

	naui__net_field_copy(r[2], naui__net_field_one);
	naui__net_field_unpack(r[1], p);
	naui__net_field_square(num, r[1]);
	naui__net_field_mul(den, num, naui__net_ed_d);
	naui__net_field_sub(num, num, r[2]);
	naui__net_field_add(den, r[2], den);

	naui__net_field_square(den2, den);
	naui__net_field_square(den4, den2);
	naui__net_field_mul(den6, den4, den2);
	naui__net_field_mul(t, den6, num);
	naui__net_field_mul(t, t, den);

	naui__net_field_pow2523(t, t);
	naui__net_field_mul(t, t, num);
	naui__net_field_mul(t, t, den);
	naui__net_field_mul(t, t, den);
	naui__net_field_mul(r[0], t, den);

	naui__net_field_square(chk, r[0]);
	naui__net_field_mul(chk, chk, den);
	if (!naui__net_field_equal(chk, num))
		naui__net_field_mul(r[0], r[0], naui__net_ed_sqrt_m1);

	naui__net_field_square(chk, r[0]);
	naui__net_field_mul(chk, chk, den);
	if (!naui__net_field_equal(chk, num))
		return false;

	if (naui__net_field_parity(r[0]) == (p[31] >> 7))
		naui__net_field_sub(r[0], naui__net_field_zero, r[0]);

	naui__net_field_mul(r[3], r[0], r[1]);
	return true;
}

static bool naui__net_ed_scalar_is_canonical(const uint8_t s[32])
{
	for (int i = 31; i >= 0; i--)
	{
		if (s[i] < naui__net_ed_order[i])
			return true;

		if (s[i] > naui__net_ed_order[i])
			return false;
	}

	return false;
}

static bool naui__net_ed25519_verify(const uint8_t public_key[32], const uint8_t* message, size_t size, const uint8_t signature[64])
{
	NetField p[4];
	NetField q[4];
	uint8_t h[64];
	uint8_t check[32];
	NetSha512 sha;

	if (!naui__net_ed_scalar_is_canonical(signature + 32))
		return false;

	if (!naui__net_ed_unpack_negated(q, public_key))
		return false;

	naui__net_sha512_init(&sha);
	naui__net_sha512_update(&sha, signature, 32);
	naui__net_sha512_update(&sha, public_key, 32);
	naui__net_sha512_update(&sha, message, size);
	naui__net_sha512_final(&sha, h);
	naui__net_ed_reduce(h);

	naui__net_ed_scalar_mul(p, q, h);
	naui__net_ed_scalar_base(q, signature + 32);
	naui__net_ed_add(p, q);
	naui__net_ed_pack(check, p);
	return naui__net_ct_equal(check, signature, 32);
}
