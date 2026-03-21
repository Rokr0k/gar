#ifndef GAR_WRITER_HPP
#define GAR_WRITER_HPP

#include "writer.h"
#include <string>

namespace gar {
class Writer {
public:
  Writer() : wr(nullptr) { wr = gar_writer_alloc(); }

  Writer(Writer &&rhs) noexcept : wr(rhs.wr) { rhs.wr = nullptr; }

  Writer &operator=(Writer &&rhs) noexcept {
    if (this != &rhs) {
      gar_writer_free(wr);
      wr = rhs.wr;
      rhs.wr = nullptr;
    }
    return *this;
  }

  ~Writer() { gar_writer_free(wr); }

  bool SetFile(const std::string &file) {
    return gar_writer_set_file(wr, file.c_str()) == 0;
  }

  bool Add(const std::string &name, const std::string &file) {
    return gar_writer_add(wr, name.c_str(), file.c_str()) == 0;
  }

  bool Finish() { return gar_writer_finish(wr) == 0; }

private:
  gar_writer_t *wr;
};
} // namespace gar

#endif
