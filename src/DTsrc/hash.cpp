#include "hash.h"

using namespace std;

uint32_t Hash16(uint32_t hash) {
	hash *= 0x85ebca6b;
	hash ^= hash >> 16;
	hash *= 0xc2b2ae35;
	hash ^= hash >> 16;
	return hash;
}

uint32_t Hash32_2(uint32_t hash1, uint32_t hash2) {
    hash1 ^= hash1 >> 16;
    hash1 *= 0x85ebca6b;
    hash1 ^= hash1 >> 13;
    hash1 *= 0xc2b2ae35;

    hash2 ^= hash2 >> 16;
    hash2 *= 0x85ebca6b;
    hash2 ^= hash2 >> 13;
    hash2 *= 0xc2b2ae35;

    hash1 ^= hash2;
    hash1 ^= hash1 >> 16;

    return hash1;
}