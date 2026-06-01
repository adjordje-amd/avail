#include "avail/input.h"

#include <cerrno>
#include <cstddef>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <utility>

namespace avail {

RawModeGuard::RawModeGuard(int fd) : m_fd(fd) {
    if (::isatty(fd) == 0) {
        return;
    }
    if (::tcgetattr(fd, &m_saved) != 0) {
        return;
    }
    termios raw = m_saved;
    raw.c_lflag &= ~(static_cast<tcflag_t>(ICANON) | static_cast<tcflag_t>(ECHO));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (::tcsetattr(fd, TCSANOW, &raw) != 0) {
        return;
    }
    m_active = true;
}

RawModeGuard::~RawModeGuard() {
    restore();
}

RawModeGuard::RawModeGuard(RawModeGuard&& other) noexcept
    : m_fd(other.m_fd), m_active(other.m_active), m_saved(other.m_saved) {
    other.m_active = false;
    other.m_fd = -1;
}

RawModeGuard& RawModeGuard::operator=(RawModeGuard&& other) noexcept {
    if (this != &other) {
        restore();
        m_fd = other.m_fd;
        m_active = other.m_active;
        m_saved = other.m_saved;
        other.m_active = false;
        other.m_fd = -1;
    }
    return *this;
}

void RawModeGuard::restore() noexcept {
    if (!m_active) {
        return;
    }
    ::tcsetattr(m_fd, TCSANOW, &m_saved);
    m_active = false;
}

namespace {

bool wait_readable(int fd, int timeout_ms) noexcept {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(fd, &set);
    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    const int rc = ::select(fd + 1, &set, nullptr, nullptr, &tv);
    return rc > 0 && FD_ISSET(fd, &set);
}

ssize_t read_byte(int fd, char& out) noexcept {
    for (;;) {
        const ssize_t n = ::read(fd, &out, 1);
        if (n == -1 && errno == EINTR) {
            continue;
        }
        return n;
    }
}

}  // namespace

KeyEvent parse_key_sequence(const char* bytes, std::size_t len) noexcept {
    if (len == 0) {
        return KeyEvent{Key::Unknown, 0};
    }
    const char b0 = bytes[0];
    if (b0 == '\r' || b0 == '\n') {
        return KeyEvent{Key::Enter, 0};
    }
    if (b0 == 0x1B) {
        if (len < 3 || bytes[1] != '[') {
            return KeyEvent{Key::Esc, 0};
        }
        switch (bytes[2]) {
        case 'A':
            return KeyEvent{Key::Up, 0};
        case 'B':
            return KeyEvent{Key::Down, 0};
        case 'C':
            return KeyEvent{Key::Right, 0};
        case 'D':
            return KeyEvent{Key::Left, 0};
        default:
            return KeyEvent{Key::Unknown, 0};
        }
    }
    return KeyEvent{Key::Char, b0};
}

KeyEvent read_key(int fd) {
    char buf[8] = {0};
    const ssize_t n0 = read_byte(fd, buf[0]);
    if (n0 <= 0) {
        return KeyEvent{Key::Unknown, 0};
    }
    if (buf[0] != 0x1B) {
        return parse_key_sequence(buf, 1);
    }
    if (!wait_readable(fd, 50)) {
        return KeyEvent{Key::Esc, 0};
    }
    if (read_byte(fd, buf[1]) <= 0 || buf[1] != '[') {
        return KeyEvent{Key::Esc, 0};
    }
    if (!wait_readable(fd, 50) || read_byte(fd, buf[2]) <= 0) {
        return KeyEvent{Key::Esc, 0};
    }
    return parse_key_sequence(buf, 3);
}

bool stdin_is_tty() noexcept {
    return ::isatty(STDIN_FILENO) != 0;
}

bool stdout_is_tty() noexcept {
    return ::isatty(STDOUT_FILENO) != 0;
}

}  // namespace avail
