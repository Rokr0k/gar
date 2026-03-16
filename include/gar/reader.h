#ifndef GAR_READER_H
#define GAR_READER_H

#include "exports.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gar_reader gar_reader_t;

GAR_API gar_reader_t *gar_reader_alloc(void);

GAR_API void gar_reader_free(gar_reader_t *rd);

GAR_API int gar_reader_open(gar_reader_t *rd, const char *file);

GAR_API int gar_reader_find(gar_reader_t *rd, const char *name, uint32_t *id);

GAR_API int gar_reader_size(gar_reader_t *rd, uint32_t id, uint64_t *size);

GAR_API int gar_reader_read(gar_reader_t *rd, uint32_t id, uint8_t *ptr,
                            uint64_t *size);

#ifdef __cplusplus
}
#endif

#endif
