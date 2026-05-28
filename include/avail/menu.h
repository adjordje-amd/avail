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
 * Arrow Up / Down rotate the selection (with wrap-around in unpaginated
 * mode, or clamped within the current page when @c set_page_size has
 * been called with a non-zero value). Render emits the list with the
 * current row highlighted; the rest of the screen (titles, headers,
 * body text) is the caller's responsibility.
 *
 * Pagination is opt-in. Default construction yields the legacy
 * wrap-around behaviour. Call @c set_page_size(n) to slice the item
 * list into pages of @p n items each. Page math:
 *  - @c total_pages() is @c 1 for empty lists (the empty first page).
 *  - @c next_page / @c prev_page clamp at the edges (no wrap).
 *  - Switching page snaps the selection to the first row on the new
 *    page so a subsequent Up / Down stays within the visible slice.
 */
class MenuController {
  public:
    explicit MenuController(std::vector<MenuItem> items, std::size_t initial = 0) noexcept;

    void move_up() noexcept;
    void move_down() noexcept;

    /**
     * Enable / reconfigure pagination.
     * @param page_size Items per page. Zero disables pagination (Up / Down
     *                  wrap around the full list, as in the legacy mode).
     */
    void set_page_size(std::size_t page_size) noexcept;
    void next_page() noexcept;
    void prev_page() noexcept;

    [[nodiscard]] std::size_t page_size() const noexcept { return m_page_size; }
    [[nodiscard]] std::size_t current_page() const noexcept;
    [[nodiscard]] std::size_t total_pages() const noexcept;
    [[nodiscard]] std::size_t page_begin() const noexcept;
    [[nodiscard]] std::size_t page_end() const noexcept;

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
    std::size_t m_page_size = 0;  // 0 means pagination disabled
};

}  // namespace avail
