#include <config.h>
#include <gar/gar.h>
#include <sodium/core.h>

const gar_version_t *gar_version = &(const gar_version_t){
    GAR_VERSION_MAJOR,
    GAR_VERSION_MINOR,
    GAR_VERSION_PATCH,
};

int gar_enc_init(void) {
  if (sodium_init() < 0) {
    return -1;
  }

  return 0;
}
