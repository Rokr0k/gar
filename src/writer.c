#include "common.h"
#include "fmap.h"
#include <gar/writer.h>
#include <sodium.h>
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
  gar_header_t header;
  uint8_t enc_key[crypto_secretbox_KEYBYTES];
};

gar_writer_t *gar_writer_alloc(void) {
  gar_writer_t *wr = malloc(sizeof(*wr));
  if (wr == NULL) {
    return NULL;
  }

  memset(wr, 0, sizeof(*wr));

  wr->header.signature = GAR_SIGNATURE;

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

int gar_writer_set_file(gar_writer_t *wr, const char *file,
                        const uint8_t *key) {
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

  if (key != NULL) {
    memcpy(wr->enc_key, key, sizeof(wr->enc_key));
  } else {
    memset(wr->enc_key, 0, sizeof(wr->enc_key));
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
      .enc_nonce = {},
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

  if (!sodium_is_zero(wr->enc_key, sizeof(wr->enc_key))) {
    randombytes_buf(entry.enc_nonce, sizeof(entry.enc_nonce));

    void *new_buffer = malloc(entry.csize + crypto_secretbox_MACBYTES);
    if (new_buffer == NULL) {
      memset(entry.enc_nonce, 0, sizeof(entry.enc_nonce));
      goto enc_fail;
    }

    if (crypto_secretbox_easy(new_buffer, buffer, entry.csize, entry.enc_nonce,
                              wr->enc_key) != 0) {
      memset(entry.enc_nonce, 0, sizeof(entry.enc_nonce));
      free(new_buffer);
      goto enc_fail;
    }

    free(buffer);
    buffer = new_buffer;
    entry.csize += crypto_secretbox_MACBYTES;
  }
enc_fail:

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
  wr->header.index_size += sizeof(entry);

  return 0;
}

int gar_writer_finish(gar_writer_t *wr) {
  if (wr == NULL) {
    return -1;
  }

  gar_entry_t *index = malloc(wr->header.index_size);
  if (index == NULL) {
    return -1;
  }

  struct entry_node *node = wr->index;
  for (size_t i = 0; node != NULL; i++, node = node->next) {
    index[i] = node->entry;
  }

  if (!sodium_is_zero(wr->enc_key, sizeof(wr->enc_key))) {
    randombytes_buf(wr->header.enc_nonce, sizeof(wr->header.enc_nonce));

    gar_entry_t *new_index =
        malloc(wr->header.index_size + crypto_secretbox_MACBYTES);
    if (new_index == NULL) {
      memset(wr->header.enc_nonce, 0, sizeof(wr->header.enc_nonce));
      goto enc_fail;
    }

    if (crypto_secretbox_easy((uint8_t *)new_index, (const uint8_t *)index,
                              wr->header.index_size, wr->header.enc_nonce,
                              wr->enc_key) != 0) {
      free(new_index);
      memset(wr->header.enc_nonce, 0, sizeof(wr->header.enc_nonce));
      goto enc_fail;
    }

    free(index);
    index = new_index;
    wr->header.index_size += crypto_secretbox_MACBYTES;
  }
enc_fail:

  wr->header.index_offset = ftell(wr->fp);

  if (fwrite(index, 1, wr->header.index_size, wr->fp) < wr->header.index_size) {
    free(index);
    return -1;
  }

  if (fseek(wr->fp, 0, SEEK_SET) != 0) {
    free(index);
    return -1;
  }

  if (fwrite(&wr->header, sizeof(wr->header), 1, wr->fp) < 1) {
    free(index);
    return -1;
  }

  free(index);

  return 0;
}
