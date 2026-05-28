# avail

Prototype UX skeleton for `rocprof-sys-avail`. Runnable C++20 REPL that
walks the proposed screen flow against mock device catalogs, so the UX
can be reviewed end-to-end before any real backend exists.

## What it is

- Interactive REPL navigating a state machine of numbered menus.
- Two mock catalogs: an "online" snapshot of a fake workstation
  (Ryzen 9950X3D + RX 9070 XT + an integrated gfx1036 + a Pensando NIC)
  and a generic "offline" catalog (one CPU / GPU / AINIC, every metric
  marked aspirational).
- Synthesizes a `rocprof-sys-run` command line from selected metrics.
- Marks aspirational entries with `[demo: proposed UX]` so reviewers
  can tell what is real today vs proposed.
- Non-interactive demo mode walks every screen in one pass.

There is no real device discovery, no real metric backend. Everything
is behind small injectable interfaces (`IDeviceCatalog`,
`IMetricCatalog`, `IRocmVersion`, `ILibraryEnv`) wired by constructor
injection.

## Build

Requires CMake >= 3.21, a C++20 compiler, network access for the first
configure (FetchContent pulls googletest v1.14.0).

```
cmake --preset default
cmake --build build --preset default
ctest --preset default --output-on-failure
```

Debug or coverage variants:

```
cmake --preset debug    && cmake --build build-debug
cmake --preset coverage && cmake --build build-coverage
```

## Run

```
./build/avail                       # interactive REPL
./build/avail --demo-frames         # walk every screen, then exit
./build/avail --demo-frames --no-color
./build/avail --version
./build/avail --help
```

## Navigate

The REPL uses numbered menus. Common keys:

- digit -> pick option / toggle metric
- `v N` -> view metric N in the current device's metric detail screen
- `c`   -> clear selections for the current device
- `b`   -> back
- `quit` / `q` -> exit

The synthesized command sits behind `Create profiling configuration` -> pick
a device -> toggle metrics -> back to session screen -> `2` (preview).

## Plug in a real provider

Each provider is a 1-method interface in `include/avail/interfaces.h`.
To wire a real backend, implement the interfaces against your data
source and pass instances by reference to the `App` constructor in
`src/main.cc`. No singletons, no service locator: lifetime stays in
`main`.

## Layout

```
include/avail/   public headers (device, session, render, screens,
                 synthesize, interfaces, mocks, app)
src/             implementations + main.cc
tests/           gtest+gmock suites (5 suites, >50 tests)
third_party/     CLI11 single-header (vendored)
```
