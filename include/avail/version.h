#pragma once

namespace avail {

/** @return Compile-time semver string for this build (e.g. "0.1.0"). */
[[nodiscard]] const char* version_string() noexcept;

}  // namespace avail
