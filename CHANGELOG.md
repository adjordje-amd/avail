# Changelog

All notable changes to this project will be documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [0.1.0] - 2026-05-28

### Added

- Initial scaffold: CMake 3.21 + C++20, `-Wall -Wextra -Wpedantic -Werror`.
- `avail_core` static library with `App`, `SessionState`, state machine
  (`screens.{h,cc}`), ANSI-aware renderer (`render.{h,cc}`), and command
  synthesizer (`synthesize.{h,cc}`).
- 4 injectable provider interfaces: `IDeviceCatalog`, `IMetricCatalog`,
  `IRocmVersion`, `ILibraryEnv`. Constructor injection only.
- Mock providers: `MockOnline*` (Ryzen 9950X3D, RX 9070 XT, integrated
  gfx1036 unsupported, Pensando DSC 200G) and `MockOffline*` (generic
  CPU/GPU/AINIC, every metric aspirational).
- CLI11-driven `avail` binary with `--version`, `--demo-frames`,
  `--no-color`, `--help`.
- gtest+gmock test suites: `test_session`, `test_screens`, `test_render`,
  `test_synthesize`, `test_mocks`, `test_demo_walker` (>50 cases).
- Pre-commit config (clang-format, trailing-whitespace, end-of-file-fixer),
  LLVM-derived `.clang-format`, MIT license.
- CMakePresets.json with `default` / `debug` / `coverage` configurations.
- README + this changelog.
