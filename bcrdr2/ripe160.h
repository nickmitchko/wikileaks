#ifndef RIPE160_H_
#define RIPE160_H_

#include <stdint.h>

typedef struct {
  uint64_t length;
  union {
    uint32_t w[16];
    uint8_t  b[64];
  } buf;
  uint32_t h[5];
  uint8_t bufpos;
} ripemd160_state;

void ripemd160_init(ripemd160_state * md);
void ripemd160_process(ripemd160_state *self, const unsigned char *p, unsigned long length);
void ripemd160_done(ripemd160_state *self, unsigned char *out);
void ripemd160(const void *in, unsigned long inlen, void *out);

#endif
