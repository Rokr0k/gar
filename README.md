# GAR

**GAR** is an archive format with efficient extraction on known entry name.
Primary purpose of use is resource packing for games.

## Dependencies

- [`xxHash`](https://github.com/Cyan4973/xxHash)
- [`zstd`](https://github.com/facebook/zstd)

You can use [`vcpkg`](https://github.com/microsoft/vcpkg)
if those dependencies are not provided by the system.

## Build & Install

```sh
cmake -S . -B build
cmake --install build --prefix=$prefix
```

## Usage

```cmake
find_package(gar CONFIG REQUIRED)

target_link_libraries(exe gar::gar)

add_custom_target(res ALL
    COMMAND gar::gartool archive "${CMAKE_CURRENT_BINARY_DIR}/res.gar" "res"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
```

```c
#include <gar/gar.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  gar_reader_t *rd = gar_reader_alloc();
  if (rd == NULL) {
    return 1;
  }

  if (gar_reader_open(rd, "res.gar") != 0) {
    return 1;
  }

  uint32_t id;
  if (gar_reader_find(rd, "res/item.txt", &id) != 0) {
    return 1;
  }

  uint64_t size;
  if (gar_reader_size(rd, id, &size) != 0) {
    return 1;
  }

  uint8_t *ptr = malloc(size);
  if (ptr == NULL) {
    return 1;
  }

  if (gar_reader_read(rd, id, ptr, &size) != 0) {
    return 1;
  }

  gar_reader_free(rd);

  // use `ptr` and `size`

  free(ptr);

  return 0;
}
```
