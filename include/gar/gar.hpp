#ifndef GAR_GAR_HPP
#define GAR_GAR_HPP

#include "gar.h"
#include "reader.hpp" // IWYU pragma: export
#include "writer.hpp" // IWYU pragma: export
#include <string>

namespace gar {
class Version {
public:
  constexpr Version(const gar_version_t *version) : version(*version) {}

  constexpr int Major() const { return version.major; }
  constexpr int Minor() const { return version.minor; }
  constexpr int Patch() const { return version.patch; }

  std::string String() const {
    return std::to_string(Major()) + "." + std::to_string(Minor()) + "." +
           std::to_string(Patch());
  }

private:
  gar_version_t version;
};

inline const Version version = gar_version;

inline bool InitEnc() { return gar_enc_init() == 0; }
} // namespace gar

#endif
