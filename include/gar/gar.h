#ifndef GAR_GAR_H
#define GAR_GAR_H

#include "exports.h"
#include "reader.h" /* IWYU pragma: export */
#include "writer.h" /* IWYU pragma: export */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gar_version {
  int major;
  int minor;
  int patch;
} gar_version_t;

extern GAR_API const gar_version_t *gar_version;

#ifdef __cplusplus
}
#endif

#endif
