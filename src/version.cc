#include "avail/version.h"

#ifndef AVAIL_VERSION_STRING
#define AVAIL_VERSION_STRING "0.0.0"
#endif

namespace avail {

const char* version_string() noexcept {
    return AVAIL_VERSION_STRING;
}

}  // namespace avail
