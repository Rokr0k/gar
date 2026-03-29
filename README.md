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
# CMakeFiles.txt

find_package(gar CONFIG REQUIRED)

string(RANDOM
    LENGTH 64
    ALPHABET 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ
    res_enc_key)

add_executable(exe main.c)
target_link_libraries(exe gar::gar)
target_compile_definitions(exe PRIVATE RES_ENC_KEY="\"${res_enc_key}\"")

add_custom_target(res ALL
    COMMAND ${CMAKE_COMMAND} -E env "GAR_ENV_KEY=${res_enc_key}" --
            gar::gartool archive "${CMAKE_CURRENT_BINARY_DIR}/res.gar" "res"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
```

```c
/* main.c */
#include <gar/gar.h>
#include <stdint.h>
#include <stdlib.h>

static void hex_to_bytes(const char *hex, uint8_t *bytes) {
  for (int i = 0; i < 32; i++) {
    if (hex[2 * i] == '\0') {
      bytes[i] = 0;
      break;
    }

    if (hex[2 * i] >= '0' && hex[2 * i] <= '9') {
      bytes[i] = (hex[2 * i] - '0') << 4;
    } else if (hex[2 * i] >= 'A' && hex[2 * i] <= 'Z') {
      bytes[i] = (hex[2 * i] - 'A' + 10) << 4;
    } else if (hex[2 * i] >= 'a' && hex[2 * i] <= 'z') {
      bytes[i] = (hex[2 * i] - 'a' + 10) << 4;
    } else {
      bytes[i] = 0;
    }

    if (hex[2 * i + 1] == '\0') {
      break;
    }

    if (hex[2 * i + 1] >= '0' && hex[2 * i + 1] <= '9') {
      bytes[i] |= hex[2 * i + 1] - '0';
    } else if (hex[2 * i + 1] >= 'A' && hex[2 * i + 1] <= 'Z') {
      bytes[i] |= hex[2 * i + 1] - 'A' + 10;
    } else if (hex[2 * i + 1] >= 'a' && hex[2 * i + 1] <= 'z') {
      bytes[i] |= hex[2 * i + 1] - 'a' + 10;
    }
  }
}

int main(int argc, char **argv) {
  gar_reader_t *rd = gar_reader_alloc();
  if (rd == NULL) {
    return 1;
  }

  uint8_t key[32] = {};
  hex_to_bytes(RES_ENC_KEY, key)
  if (gar_reader_open(rd, "res.gar", key) != 0) {
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
