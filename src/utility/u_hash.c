#include "u_utility.h"

#define HASH_BITS_LONG_LONG ((sizeof (size_t)) * 8)
#define HASH_ROTATE_LEFT(val, n)   (((val) << (n)) | ((val) >> (HASH_BITS_LONG_LONG - (n))))
#define HASH_ROTATE_RIGHT(val, n)  (((val) >> (n)) | ((val) << (HASH_BITS_LONG_LONG - (n))))
size_t Hash_stringSeeded(char* p_str, size_t p_seed, unsigned p_max)
{
	//src: http://web.archive.org/web/20071223173210/http://www.concentric.net/~Ttwang/tech/inthash.htm
	size_t hash = p_seed;

	while (*p_str)
	{
		hash = HASH_ROTATE_LEFT(hash, 9) + (unsigned char)*p_str++;
	}
	hash ^= p_seed;
	const int c2 = 0x27d4eb2d;
	hash = (hash ^ 61) ^ (hash >> 16);
	hash = hash + (hash << 3);
	hash = hash ^ (hash >> 4);
	hash = hash * c2;
	hash = hash ^ (hash >> 15);

	return hash + p_seed;
}

size_t Hash_string(char* p_str)
{
	//src: http://web.archive.org/web/20071223173210/http://www.concentric.net/~Ttwang/tech/inthash.htm
	size_t hash = 0;

	while (*p_str)
	{
		hash = HASH_ROTATE_LEFT(hash, 9) + (unsigned char)*p_str++;
	}
	const int c2 = 0x27d4eb2d;
	hash = (hash ^ 61) ^ (hash >> 16);
	hash = hash + (hash << 3);
	hash = hash ^ (hash >> 4);
	hash = hash * c2;
	hash = hash ^ (hash >> 15);

	return hash;
}

uint32_t Hash_string2(const char* p_str)
{
	//src: https://theartincode.stanis.me/008-djb2/

	const unsigned char* chr = (const unsigned char*)p_str;

	uint32_t hash = 5381;
	uint32_t c = *chr++;

	while (c)
	{
		hash = ((hash << 5) + hash) ^ c; /* hash * 33 ^ c */
		c = *chr++;
	}

	return hash;
}



//src https://github.com/godotengine/godot/blob/master/core/templates/hashfuncs.h
static uint32_t hash_fmix32(uint32_t h)
{
	h ^= h >> 16;
	h *= 0x85ebca6b;
	h ^= h >> 13;
	h *= 0xc2b2ae35;
	h ^= h >> 16;

	return h;
}
#define HASH_MURMUR3_SEED 0x7F07C65
// Murmurhash3 32-bit version.
// All MurmurHash versions are public domain software, and the author disclaims all copyright to their code.

uint32_t Hash_uint32(uint32_t p_in, uint32_t p_seed)
{
	p_in *= 0xcc9e2d51;
	p_in = (p_in << 15) | (p_in >> 17);
	p_in *= 0x1b873593;

	p_seed ^= p_in;
	p_seed = (p_seed << 13) | (p_seed >> 19);
	p_seed = p_seed * 5 + 0xe6546b64;

	return p_seed;
}

uint32_t Hash_float(float p_in, uint32_t p_seed)
{
	union {
		float f;
		uint32_t i;
	} u;

	// Normalize +/- 0.0 and NaN values so they hash the same.
	if (p_in == 0.0f) {
		u.f = 0.0;
	}
	else if (isnan(p_in)) {
		u.f = NAN;
	}
	else {
		u.f = p_in;
	}

	return Hash_uint32(u.i, p_seed);
}

uint32_t Hash_vec3(vec3 v)
{
	uint32_t h = Hash_float(v[0], HASH_MURMUR3_SEED);
	h = Hash_float(v[1], h);
	h = Hash_float(v[2], h);
	return hash_fmix32(h);
}
uint32_t Hash_ivec3(ivec3 v)
{
	uint32_t h = Hash_uint32(v[0], HASH_MURMUR3_SEED);
	h = Hash_uint32(v[1], h);
	h = Hash_uint32(v[2], h);
	return hash_fmix32(h);
}

uint32_t Hash_id(uint32_t x)
{
	x = ((x >> (uint32_t)(16)) ^ x) * (uint32_t)(0x45d9f3b);
	x = ((x >> (uint32_t)(16)) ^ x) * (uint32_t)(0x45d9f3b);
	x = (x >> (uint32_t)(16)) ^ x;
	return x;
}

uint32_t Hash_uint64(uint64_t x)
{
	uint64_t v = x;
	v = (~v) + (v << 18); // v = (v << 18) - v - 1;
	v = v ^ (v >> 31);
	v = v * 21; // v = (v + (v << 2)) + (v << 4);
	v = v ^ (v >> 11);
	v = v + (v << 6);
	v = v ^ (v >> 22);

	return v;
}
