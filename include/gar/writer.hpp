#ifndef GAR_WRITER_HPP
#define GAR_WRITER_HPP

#include "writer.h"
#include <string>

namespace gar {
class Writer {
public:
  Writer() noexcept : wr(nullptr) { wr = gar_writer_alloc(); }

  Writer(Writer &&rhs) noexcept : wr(rhs.wr) { rhs.wr = nullptr; }

  Writer &operator=(Writer &&rhs) noexcept {
    if (this != &rhs) {
      gar_writer_free(wr);
      wr = rhs.wr;
      rhs.wr = nullptr;
    }
    return *this;
  }

  ~Writer() noexcept { gar_writer_free(wr); }

  void SetOption(const gar_writer_option_t &option) noexcept {
    gar_writer_set_option(wr, &option);
  }

  void SetKey(const uint8_t *key) noexcept { gar_writer_set_enc_key(wr, key); }

  bool SetFile(const std::string &file) noexcept {
    return gar_writer_set_file(wr, file.c_str()) == 0;
  }

  bool Add(const std::string &name, const std::string &file) noexcept {
    return gar_writer_add_file(wr, name.c_str(), file.c_str()) == 0;
  }

  bool Add(const std::string &name, const uint8_t *ptr, size_t size) noexcept {
    return gar_writer_add_memory(wr, name.c_str(), ptr, size) == 0;
  }

  template <class Iterator>
  bool Add(const std::string &name, Iterator begin, Iterator end) noexcept {
    return gar_writer_add_memory(wr, name.c_str(), &*begin,
                                 std::distance(begin, end)) == 0;
  }

  bool Finish() noexcept { return gar_writer_finish(wr) == 0; }

private:
  gar_writer_t *wr;
};
} // namespace gar

#endif
