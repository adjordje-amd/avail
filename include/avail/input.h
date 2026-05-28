#pragma once

#include <cstddef>
#include <termios.h>
#include <unistd.h>

namespace avail {

/**
 * RAII guard for terminal raw (non-canonical, no-echo) mode.
 *
 * Ctor calls @c tcgetattr to snapshot the current termios, then applies
 * a mask that disables @c ICANON and @c ECHO. Dtor restores the snapshot.
 *
 * Move-only. The moved-from instance no longer owns the restore action.
 *
 * @note Construction is a no-op (with @c active()==false) when @c fd is
 *       not a TTY, so callers can construct unconditionally in code paths
 *       that might be piped.
 */
class RawModeGuard {
  public:
    explicit RawModeGuard(int fd = STDIN_FILENO);
    ~RawModeGuard();

    RawModeGuard(const RawModeGuard&) = delete;
    RawModeGuard& operator=(const RawModeGuard&) = delete;

    RawModeGuard(RawModeGuard&& other) noexcept;
    RawModeGuard& operator=(RawModeGuard&& other) noexcept;

    [[nodiscard]] bool active() const noexcept { return m_active; }

    /** Restore cooked mode early. Safe to call from a signal handler. */
    void restore() noexcept;

  private:
    int m_fd = -1;
    bool m_active = false;
    termios m_saved{};
};

/** Enumerated keys the REPL understands. */
enum class Key {
    Up,
    Down,
    Left,
    Right,
    Enter,
    Esc,
    Char,
    Unknown,
};

/**
 * Single keypress decoded from raw input.
 *
 * @c ch is populated only when @c key == Key::Char; otherwise it is 0.
 */
struct KeyEvent {
    Key key = Key::Unknown;
    char ch = 0;
};

/**
 * Read and decode one keypress from @c fd.
 *
 * Blocks until a byte is available. If the leading byte is @c 0x1B,
 * uses @c select() with a 50ms timeout to look for a @c "[A"-style
 * arrow-key follow-up; if no follow-up appears, returns a bare
 * @c Key::Esc.
 *
 * @return KeyEvent describing the decoded key.
 */
[[nodiscard]] KeyEvent read_key(int fd = STDIN_FILENO);

/** Decode a byte sequence as if it had just been read. Pure, for tests. */
[[nodiscard]] KeyEvent parse_key_sequence(const char* bytes, std::size_t len) noexcept;

[[nodiscard]] bool stdin_is_tty() noexcept;
[[nodiscard]] bool stdout_is_tty() noexcept;

/** ANSI clear-screen + cursor-home sequence. */
inline constexpr const char* kClearScreen = "\x1b[2J\x1b[H";

}  // namespace avail
