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
  gar_writer_option_t option;
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

void gar_writer_set_option(gar_writer_t *wr,
                           const gar_writer_option_t *option) {
  if (wr == NULL) {
    return;
  }

  if (option == NULL) {
    wr->option = (gar_writer_option_t){};
  } else {
    wr->option = *option;
  }
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

int gar_writer_add_file(gar_writer_t *wr, const char *name, const char *file) {
  if (wr == NULL || name == NULL || file == NULL) {
    return -1;
  }

  gar_fmap_t fmap;
  if (gar_fmap_map(&fmap, file) != 0) {
    return -1;
  }

  if (gar_writer_add_memory(wr, name, fmap.ptr, fmap.size) != 0) {
    gar_fmap_unmap(&fmap);
    return -1;
  }

  gar_fmap_unmap(&fmap);

  return 0;
}

int gar_writer_add_memory(gar_writer_t *wr, const char *name, const void *ptr,
                          uint64_t size) {
  if (wr == NULL || name == NULL || ptr == NULL || size == 0) {
    return -1;
  }

  gar_entry_t entry = {
      .key = XXH3_64bits(name, strlen(name)),
      .offset = ftell(wr->fp),
      .usize = size,
      .csize = ZSTD_compressBound(size),
  };

  void *buffer = malloc(entry.csize);
  if (buffer == NULL) {
    return -1;
  }

  uint64_t csize = ZSTD_compress(buffer, entry.csize, ptr, entry.usize,
                                 wr->option.compression_level);
  if (ZSTD_isError(csize)) {
    free(buffer);
    return -1;
  }

  entry.csize = csize;

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
