#include "common.h"
#include "fmap.h"
#include <gar/reader.h>
#include <sodium.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xxhash.h>
#include <zstd.h>

struct gar_reader {
  gar_fmap_t src;
  const gar_header_t *header;
  gar_entry_t *index;
  size_t index_count;
  uint8_t enc_key[crypto_secretbox_KEYBYTES];
};

gar_reader_t *gar_reader_alloc(void) {
  gar_reader_t *rd = malloc(sizeof(*rd));
  if (rd == NULL) {
    return NULL;
  }

  memset(rd, 0, sizeof(*rd));

  return rd;
}

void gar_reader_free(gar_reader_t *rd) {
  if (rd == NULL) {
    return;
  }

  if (!sodium_is_zero(rd->header->enc_nonce, sizeof(rd->header->enc_nonce))) {
    free(rd->index);
  }

  gar_fmap_unmap(&rd->src);

  free(rd);
}

int gar_reader_open(gar_reader_t *rd, const char *file, const uint8_t *key) {
  if (rd == NULL || file == NULL) {
    return -1;
  }

  if (!sodium_is_zero(rd->header->enc_nonce, sizeof(rd->header->enc_nonce))) {
    free(rd->index);
  }

  gar_fmap_unmap(&rd->src);

  if (gar_fmap_map(&rd->src, file) != 0) {
    return -1;
  }

  rd->header = (const gar_header_t *)rd->src.ptr;
  if (rd->header->signature != GAR_SIGNATURE) {
    gar_fmap_unmap(&rd->src);
    return -1;
  }

  if (key != NULL) {
    memcpy(rd->enc_key, key, sizeof(rd->enc_key));
  }

  if (sodium_is_zero(rd->header->enc_nonce, sizeof(rd->header->enc_nonce))) {
    rd->index = (gar_entry_t *)(rd->src.ptr + rd->header->index_offset);
    rd->index_count = rd->header->index_size / sizeof(*rd->index);
  } else {
    rd->index = malloc(rd->header->index_size - crypto_secretbox_MACBYTES);
    if (rd->index == NULL) {
      gar_fmap_unmap(&rd->src);
      return -1;
    }

    if (crypto_secretbox_open_easy(
            (uint8_t *)rd->index, rd->src.ptr + rd->header->index_offset,
            rd->header->index_size, rd->header->enc_nonce, rd->enc_key) != 0) {
      free(rd->index);
      gar_fmap_unmap(&rd->src);
      return -1;
    }

    rd->index_count = (rd->header->index_size - crypto_secretbox_MACBYTES) /
                      sizeof(*rd->index);
  }

  return 0;
}

int gar_reader_find(gar_reader_t *rd, const char *name, uint32_t *id) {
  if (rd == NULL || name == NULL || id == NULL) {
    return -1;
  }

  uint64_t key = XXH3_64bits(name, strlen(name));

  for (uint32_t i = 0; i < rd->index_count; i++) {
    if (rd->index[i].key == key) {
      if (id != NULL) {
        *id = i;
      }
      return 0;
    }
  }

  return -1;
}

int gar_reader_size(gar_reader_t *rd, uint32_t id, uint64_t *size) {
  if (rd == NULL || size == NULL) {
    return -1;
  }

  if (id >= rd->index_count) {
    return -1;
  }

  gar_entry_t entry = rd->index[id];

  *size = entry.usize;
  return 0;
}

int gar_reader_read(gar_reader_t *rd, uint32_t id, uint8_t *ptr,
                    uint64_t *size) {
  if (rd == NULL || ptr == NULL || size == NULL) {
    return -1;
  }

  if (id >= rd->index_count) {
    return -1;
  }

  gar_entry_t entry = rd->index[id];

  uint8_t *buffer = rd->src.ptr + entry.offset;
  uint64_t csize = entry.csize;
  int free_the_buffer = 0;

  if (!sodium_is_zero(entry.enc_nonce, sizeof(entry.enc_nonce))) {
    uint8_t *new_buffer = malloc(csize - crypto_secretbox_MACBYTES);
    if (new_buffer == NULL) {
      return -1;
    }

    if (crypto_secretbox_open_easy(new_buffer, buffer, csize, entry.enc_nonce,
                                   rd->enc_key) != 0) {
      free(new_buffer);
      return -1;
    }

    buffer = new_buffer;
    csize -= crypto_secretbox_MACBYTES;
    free_the_buffer = 1;
  }

  uint64_t usize = ZSTD_decompress(ptr, entry.usize, buffer, csize);
  if (ZSTD_isError(usize)) {
    return 0;
  }

  if (free_the_buffer) {
    free(buffer);
  }

  *size = usize;
  return 0;
}
