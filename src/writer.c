#include "common.h"
#include "fmap.h"
#include <gar/writer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xxhash.h>
#include <zstd.h>

struct entry_node {
  gar_entry_t entry;
  struct entry_node *next;
};

struct gar_writer {
  FILE *fp;
  struct entry_node *index;
};

gar_writer_t *gar_writer_alloc(void) {
  gar_writer_t *wr = malloc(sizeof(*wr));
  if (wr == NULL) {
    return NULL;
  }

  memset(wr, 0, sizeof(*wr));

  return wr;
}

void gar_writer_free(gar_writer_t *wr) {
  if (wr == NULL) {
    return;
  }

  if (wr->fp != NULL) {
    fclose(wr->fp);
  }

  while (wr->index != NULL) {
    struct entry_node *next = wr->index->next;
    free(wr->index);
    wr->index = next;
  }

  free(wr);
}

int gar_writer_set_file(gar_writer_t *wr, const char *file) {
  if (wr == NULL || file == NULL) {
    return -1;
  }

  wr->fp = fopen(file, "wb");
  if (wr->fp == NULL) {
    return -1;
  }

  if (fseek(wr->fp, sizeof(gar_header_t), SEEK_SET) != 0) {
    return -1;
  }

  return 0;
}

int gar_writer_add(gar_writer_t *wr, const char *name, const char *file) {
  if (wr == NULL || name == NULL || file == NULL) {
    return -1;
  }

  gar_fmap_t fmap;
  if (gar_fmap_map(&fmap, file) != 0) {
    return -1;
  }

  gar_entry_t entry = {
      .key = XXH3_64bits(name, strlen(name)),
      .offset = ftell(wr->fp),
      .usize = fmap.size,
      .csize = ZSTD_compressBound(fmap.size),
  };

  void *buffer = malloc(entry.csize);
  if (buffer == NULL) {
    gar_fmap_unmap(&fmap);
    return -1;
  }

  uint64_t size = ZSTD_compress(buffer, entry.csize, fmap.ptr, entry.usize, 18);
  if (ZSTD_isError(size)) {
    gar_fmap_unmap(&fmap);
    free(buffer);
    return -1;
  }

  gar_fmap_unmap(&fmap);

  entry.csize = size;

  if (fwrite(buffer, 1, entry.csize, wr->fp) < entry.csize) {
    fseek(wr->fp, entry.offset, SEEK_SET);
    free(buffer);
    return -1;
  }

  free(buffer);

  struct entry_node *node = malloc(sizeof(*node));
  if (node == NULL) {
    fseek(wr->fp, entry.offset, SEEK_SET);
    return -1;
  }

  node->entry = entry;
  node->next = wr->index;
  wr->index = node;

  return 0;
}

int gar_writer_finish(gar_writer_t *wr) {
  if (wr == NULL) {
    return -1;
  }

  gar_header_t header = {
      .signature = GAR_SIGNATURE,
      .index_count = 0,
      .index_offset = ftell(wr->fp),
  };

  struct entry_node *node = wr->index;
  while (node != NULL) {
    if (fwrite(&node->entry, sizeof(node->entry), 1, wr->fp) < 1) {
      fseek(wr->fp, header.index_offset, SEEK_SET);
      return -1;
    }
    header.index_count++;
    node = node->next;
  }

  if (fseek(wr->fp, 0, SEEK_SET) != 0) {
    return -1;
  }

  if (fwrite(&header, sizeof(header), 1, wr->fp) < 1) {
    return -1;
  }

  return 0;
}
