#include "common.h"
#include "fmap.h"
#include <gar/reader.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <xxhash.h>
#include <zstd.h>

struct gar_reader {
  gar_fmap_t src;
  const gar_header_t *header;
  const gar_entry_t *index;
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

  gar_fmap_unmap(&rd->src);

  free(rd);
}

int gar_reader_open(gar_reader_t *rd, const char *file) {
  if (rd == NULL || file == NULL) {
    return -1;
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

  rd->index = (const gar_entry_t *)(rd->src.ptr + rd->header->index_offset);

  return 0;
}

int gar_reader_find(gar_reader_t *rd, const char *name, uint32_t *id) {
  if (rd == NULL || name == NULL || id == NULL) {
    return -1;
  }

  uint64_t key = XXH3_64bits(name, strlen(name));

  for (uint32_t i = 0; i < rd->header->index_count; i++) {
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

  if (id >= rd->header->index_count) {
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

  if (id >= rd->header->index_count) {
    return -1;
  }

  gar_entry_t entry = rd->index[id];

  uint64_t usize = ZSTD_decompress(ptr, entry.usize, rd->src.ptr + entry.offset,
                                   entry.csize);
  if (ZSTD_isError(usize)) {
    return 0;
  }

  *size = usize;
  return 0;
}
