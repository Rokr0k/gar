#include "fmap.h"

#if defined(__unix__)

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int gar_fmap_map(gar_fmap_t *fmap, const char *file) {
  if (file == NULL || fmap == NULL) {
    return -1;
  }

  fmap->ptr = NULL;
  fmap->size = 0;

  int fd = open(file, O_RDONLY);
  if (fd == -1) {
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) != 0) {
    close(fd);
    return -1;
  }

  uint64_t size = st.st_size;

  void *ptr = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (ptr == MAP_FAILED) {
    close(fd);
    return -1;
  }

  fmap->ptr = ptr;
  fmap->size = size;

  return 0;
}

void gar_fmap_unmap(gar_fmap_t *fmap) {
  if (fmap == NULL || fmap->ptr == NULL) {
    return;
  }

  munmap(fmap->ptr, fmap->size);

  fmap->ptr = NULL;
  fmap->size = 0;
}

#elif defined(_WIN32)

#include <windows.h>

int gar_fmap_map(gar_fmap_t *fmap, const char *file) {
  if (file == NULL || fmap == NULL) {
    return -1;
  }

  fmap->ptr = NULL;
  fmap->size = 0;

  HANDLE hFile = CreateFileA(file, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_READONLY, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    return -1;
  }

  uint64_t size;
  if (GetFileSizeEx(hFile, (PLARGE_INTEGER)&size) == 0) {
    CloseHandle(hFile);
    return -1;
  }

  HANDLE hFileMap = CreateFileMappingA(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
  if (hFileMap == NULL) {
    CloseHandle(hFile);
    return -1;
  }

  void *ptr = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
  if (ptr == NULL) {
    CloseHandle(hFileMap);
    CloseHandle(hFile);
    return -1;
  }

  fmap->ptr = ptr;
  fmap->size = size;

  CloseHandle(hFileMap);
  CloseHandle(hFile);

  return 0;
}

void gar_fmap_unmap(gar_fmap_t *fmap) {
  if (fmap == NULL || fmap->ptr == NULL) {
    return;
  }

  UnmapViewOfFile(fmap->ptr);

  fmap->ptr = NULL;
  fmap->size = 0;
}

#else

#error "No implementation available."

#endif
