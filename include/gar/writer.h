#ifndef GAR_WRITER_H
#define GAR_WRITER_H

#include "exports.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gar_writer gar_writer_t;

GAR_API gar_writer_t *gar_writer_alloc(void);

GAR_API void gar_writer_free(gar_writer_t *wr);

GAR_API int gar_writer_set_file(gar_writer_t *wr, const char *file);

GAR_API int gar_writer_add_file(gar_writer_t *wr, const char *name,
                                const char *file);

GAR_API int gar_writer_add_memory(gar_writer_t *wr, const char *name,
                                  const void *ptr, uint64_t size);

GAR_API int gar_writer_finish(gar_writer_t *wr);

#ifdef __cplusplus
}
#endif

#endif
