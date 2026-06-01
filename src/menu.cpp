#include "avail/menu.h"

#include <cstddef>
#include <ostream>
#include <utility>

namespace avail {

MenuController::MenuController(std::vector<MenuItem> items, std::size_t initial) noexcept
    : m_items(std::move(items)), m_index(initial) {
    if (m_items.empty()) {
        m_index = 0;
        return;
    }
    if (m_index >= m_items.size()) {
        m_index = 0;
    }
}

void MenuController::move_up() noexcept {
    if (m_items.empty()) {
        return;
    }
    if (m_page_size == 0) {
        m_index = (m_index == 0) ? m_items.size() - 1 : m_index - 1;
        return;
    }
    if (m_index > page_begin()) {
        --m_index;
    }
}

void MenuController::move_down() noexcept {
    if (m_items.empty()) {
        return;
    }
    if (m_page_size == 0) {
        m_index = (m_index + 1) % m_items.size();
        return;
    }
    if (m_index + 1 < page_end()) {
        ++m_index;
    }
}

void MenuController::set_page_size(std::size_t page_size) noexcept {
    m_page_size = page_size;
    if (m_page_size != 0 && !m_items.empty() && m_index >= m_items.size()) {
        m_index = 0;
    }
}

void MenuController::next_page() noexcept {
    if (m_page_size == 0 || m_items.empty()) {
        return;
    }
    const std::size_t total = total_pages();
    const std::size_t cur = current_page();
    if (cur + 1 >= total) {
        return;
    }
    m_index = (cur + 1) * m_page_size;
}

void MenuController::prev_page() noexcept {
    if (m_page_size == 0 || m_items.empty()) {
        return;
    }
    const std::size_t cur = current_page();
    if (cur == 0) {
        return;
    }
    m_index = (cur - 1) * m_page_size;
}

std::size_t MenuController::current_page() const noexcept {
    if (m_page_size == 0) {
        return 0;
    }
    return m_index / m_page_size;
}

std::size_t MenuController::total_pages() const noexcept {
    if (m_page_size == 0 || m_items.empty()) {
        return 1;
    }
    return (m_items.size() + m_page_size - 1) / m_page_size;
}

std::size_t MenuController::page_begin() const noexcept {
    if (m_page_size == 0) {
        return 0;
    }
    return current_page() * m_page_size;
}

std::size_t MenuController::page_end() const noexcept {
    if (m_page_size == 0) {
        return m_items.size();
    }
    const std::size_t end = page_begin() + m_page_size;
    return end < m_items.size() ? end : m_items.size();
}

void MenuController::render(std::ostream& out, const RenderConfig& cfg) const {
    if (m_items.empty()) {
        return;
    }
    const std::size_t first = page_begin();
    const std::size_t last = page_end();
    for (std::size_t i = first; i < last; ++i) {
        const bool selected = (i == m_index);
        if (cfg.color) {
            if (selected) {
                out << "\x1b[7m" << m_items[i].label << "\x1b[27m";
            } else {
                out << m_items[i].label;
            }
        } else {
            out << (selected ? "> " : "  ") << m_items[i].label;
        }
        out << '\n';
    }
}

}  // namespace avail
