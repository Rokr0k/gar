#ifndef GAR_READER_HPP
#define GAR_READER_HPP

#include "reader.h"
#include <string>
#include <vector>

namespace gar {
class Reader {
public:
  Reader() : rd(nullptr) { rd = gar_reader_alloc(); }

  Reader(Reader &&rhs) noexcept : rd(rhs.rd) { rhs.rd = nullptr; }

  Reader &operator=(Reader &&rhs) noexcept {
    if (this != &rhs) {
      gar_reader_free(rd);
      rd = rhs.rd;
      rhs.rd = nullptr;
    }
    return *this;
  }

  ~Reader() { gar_reader_free(rd); }

  bool Open(const std::string &file) {
    return gar_reader_open(rd, file.c_str()) == 0;
  }

  bool Find(const std::string &name) const {
    return gar_reader_find(rd, name.c_str(), nullptr) == 0;
  }

  std::vector<uint8_t> Read(const std::string &name) const {
    uint32_t id;
    if (gar_reader_find(rd, name.c_str(), &id) != 0) {
      return {};
    }

    uint64_t size;
    if (gar_reader_size(rd, id, &size) != 0) {
      return {};
    }

    std::vector<uint8_t> buffer(size);
    if (gar_reader_read(rd, id, buffer.data(), &size) != 0) {
      return {};
    }

    buffer.resize(size);
    return buffer;
  }

private:
  gar_reader_t *rd;
};
} // namespace gar

#endif
