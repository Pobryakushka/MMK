#include <libKriging/version.hpp>

namespace libKriging {
const char* const internal_version = "0.8.3";
const char* const internal_buildTag = "-128-NOTFOUND";
LIBKRIGING_EXPORT std::string version() {
  return internal_version;
}
LIBKRIGING_EXPORT std::string buildTag() {
  return internal_buildTag;
}
}  // namespace libKriging
