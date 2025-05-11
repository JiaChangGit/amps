#ifndef MHASH_H
#define MHASH_H

#include "elementary_DT_MT.h"

using namespace std;

inline uint32_t HashKeys(uint32_t *keys, uint32_t keys_num) {
  uint32_t hash = keys[0];
  for (int i = 1; i < keys_num; ++i) hash = hash * 1000000007 + keys[i];
  return hash;
}
#endif
