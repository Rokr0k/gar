#ifndef GAR_FMAP_H
#define GAR_FMAP_H

#include <stdint.h>

typedef struct gar_fmap {
  uint8_t *ptr;
  uint64_t size;
} gar_fmap_t;

int gar_fmap_map(gar_fmap_t *fmap, const char *file);
void gar_fmap_unmap(gar_fmap_t *fmap);

#endif
