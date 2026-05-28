#pragma once

#include "avail/render.h"

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace avail {

/**
 * A single selectable row in a menu list.
 *
 * @c label is the user-visible text. @c id is a stable, opaque token the
 * dispatcher uses to identify the chosen action; it never appears on screen.
 * @c description is an optional one-line caption rendered next to the label
 * by chrome-style menu renderers (e.g. the main menu's 2-column layout);
 * default-empty so menus that do not need it are unaffected.
 */
struct MenuItem {
    std::string label;
    std::string id;
    std::string description = {};

    MenuItem() = default;
    MenuItem(std::string label_in, std::string id_in)
        : label(std::move(label_in)), id(std::move(id_in)) {}
    MenuItem(std::string label_in, std::string id_in, std::string description_in)
        : label(std::move(label_in)), id(std::move(id_in)), description(std::move(description_in)) {
    }
};

/**
 * Pure, termios-free menu state + renderer.
 *
 * Holds an ordered list of @c MenuItem and a current selection index.
 * Arrow Up / Down rotate the selection with wrap-around. Render emits
 * the list with the current row highlighted; the rest of the screen
 * (titles, headers, body text) is the caller's responsibility.
 */
class MenuController {
  public:
    explicit MenuController(std::vector<MenuItem> items, std::size_t initial = 0) noexcept;

    void move_up() noexcept;
    void move_down() noexcept;

    [[nodiscard]] std::size_t selected_index() const noexcept { return m_index; }
    [[nodiscard]] const MenuItem& current() const noexcept { return m_items[m_index]; }
    [[nodiscard]] const std::vector<MenuItem>& items() const noexcept { return m_items; }
    [[nodiscard]] bool empty() const noexcept { return m_items.empty(); }

    /**
     * Render the menu list.
     *
     * When @c cfg.color is true, the selected row is emitted with inverse
     * video (CSI 7m / 27m). When false, the selected row is prefixed with
     * @c "> " and unselected rows with @c "  " to preserve column alignment.
     */
    void render(std::ostream& out, const RenderConfig& cfg) const;

  private:
    std::vector<MenuItem> m_items;
    std::size_t m_index = 0;
};

}  // namespace avail
