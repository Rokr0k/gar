#ifndef GAR_COMMON_H
#define GAR_COMMON_H

#include <sodium/crypto_secretbox.h>
#include <stdint.h>

#define GAR_SIGNATURE 0x53466167

#define GAR_FLAG_ENC 0x01

typedef struct gar_header {
  uint32_t signature;
  uint8_t enc_nonce[crypto_secretbox_NONCEBYTES];
  uint64_t index_size;
  uint64_t index_offset;
} gar_header_t;

typedef struct gar_entry {
  uint64_t key;
  uint64_t offset;
  uint64_t csize;
  uint64_t usize;
  uint8_t enc_nonce[crypto_secretbox_NONCEBYTES];
} gar_entry_t;

#endif
