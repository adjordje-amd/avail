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
    m_index = (m_index == 0) ? m_items.size() - 1 : m_index - 1;
}

void MenuController::move_down() noexcept {
    if (m_items.empty()) {
        return;
    }
    m_index = (m_index + 1) % m_items.size();
}

void MenuController::render(std::ostream& out, const RenderConfig& cfg) const {
    if (m_items.empty()) {
        return;
    }
    for (std::size_t i = 0; i < m_items.size(); ++i) {
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
