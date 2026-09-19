#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#

"""
Reusable scrollable list widget with selection and filtering support.

Provides a list widget that can display grouped items, supports selection,
filtering, and scrolling through large lists.
"""


class SelectableListItem:
    """
    A single item in the selectable list.

    Attributes:
        name: Display name of the item
        group: Group/category name (for grouping items)
        data: Arbitrary data payload for callbacks
    """

    def __init__(self, name, group="", data=None):
        self.name = name
        self.group = group
        self.data = data if data is not None else name


class SelectableList:
    """
    Scrollable list with selection and filtering.

    Features:
    - Grouped items with headers
    - Selection highlighting
    - Keyboard navigation (up/down with scrolling)
    - Text-based filtering with swappable algorithms
    - Automatic scroll-to-selected
    """

    def __init__(self, max_visible_items=10):
        """
        Initialize the selectable list.

        Args:
            max_visible_items: Maximum number of items to display at once
        """
        self.items = []
        self.filtered_items = []
        self.selected_index = 0
        self.scroll_offset = 0
        self.max_visible_items = max_visible_items
        self.default_visible_items = (
            max_visible_items  # Track default for constant height
        )
        self.filter_text = ""
        self.filter_fn = self.substring_filter

        # Visual configuration
        self.font_size = 16.0
        self.item_height = 24.0
        self.group_header_height = 24.0  # Same as item_height for constant sizing
        self.padding = 8.0
        self.item_color = (0.85, 0.85, 0.85, 1.0)
        self.selected_color = (0.3, 0.5, 0.8, 1.0)
        self.group_header_color = (0.6, 0.6, 0.7, 1.0)
        self.group_header_bg_color = (
            0.08,
            0.08,
            0.10,
            0.95,
        )  # Slightly darker background
        self.background_color = (0.1, 0.1, 0.12, 0.95)

        # Mouse hover support
        self.hover_index = -1
        self.bounds = None  # (x, y, width, height) of last render

    def set_items(self, items):
        """
        Populate the list with items.

        Args:
            items: List of SelectableListItem objects
        """
        self.items = items
        self.apply_filter()

    def set_filter(self, text):
        """
        Apply a filter to the list.

        Args:
            text: Filter text to match against item names
        """
        self.filter_text = text.lower()
        self.apply_filter()

        # Reset selection to first item
        self.selected_index = 0
        self.scroll_offset = 0

    def apply_filter(self):
        """Apply the current filter to items."""
        if not self.filter_text:
            self.filtered_items = self.items[:]
        else:
            self.filtered_items = [
                item
                for item in self.items
                if self.filter_fn(self.filter_text, item.name.lower())
            ]

    def _items_fitting_in_rows(self, start, max_rows):
        """Count how many items fit starting from `start` within `max_rows` rows.

        Group headers consume an extra row when a new group begins.

        Returns:
            Number of items (not rows) that can be rendered.
        """
        rows_used = 0
        items_fit = 0
        last_group = None
        for i in range(start, len(self.filtered_items)):
            item = self.filtered_items[i]
            rows_needed = 1  # the item itself
            if item.group and item.group != last_group:
                rows_needed += 1  # group header
            if rows_used + rows_needed > max_rows:
                break
            rows_used += rows_needed
            last_group = item.group
            items_fit += 1
        return items_fit

    def _total_rows_for_items(self, start, end):
        """Count total rendered rows (items + group headers) for a range.

        Args:
            start: Starting item index (inclusive)
            end: Ending item index (exclusive)

        Returns:
            Total number of rows including group headers.
        """
        end = min(end, len(self.filtered_items))
        rows = 0
        last_group = None
        for i in range(start, end):
            item = self.filtered_items[i]
            if item.group and item.group != last_group:
                rows += 1  # group header
                last_group = item.group
            rows += 1  # the item
        return rows

    def move_selection(self, delta):
        """
        Move the selection up or down.

        Args:
            delta: Number of items to move (-1 for up, 1 for down)
        """
        if not self.filtered_items:
            return

        self.selected_index = max(
            0, min(self.selected_index + delta, len(self.filtered_items) - 1)
        )

        # Auto-scroll to keep selection visible.
        # Group headers consume rows, so fewer items fit than max_visible_items.
        if self.selected_index < self.scroll_offset:
            self.scroll_offset = self.selected_index
        else:
            visible_item_count = self._items_fitting_in_rows(
                self.scroll_offset, self.max_visible_items
            )
            if self.selected_index >= self.scroll_offset + visible_item_count:
                # Scroll forward until the selected item fits in the window
                while self.scroll_offset < self.selected_index:
                    self.scroll_offset += 1
                    fit = self._items_fitting_in_rows(
                        self.scroll_offset, self.max_visible_items
                    )
                    if self.selected_index < self.scroll_offset + fit:
                        break

    def scroll(self, steps):
        """
        Scroll the list by a number of items.

        Args:
            steps: Number of items to scroll (positive = down, negative = up)
        """
        if not self.filtered_items:
            return

        # Find max scroll offset: the last offset where all remaining items
        # (plus their group headers) still exceed the visible row budget,
        # meaning there's content below to scroll to.
        total_items = len(self.filtered_items)
        max_scroll = 0
        for offset in range(total_items):
            if (
                self._items_fitting_in_rows(offset, self.max_visible_items)
                >= total_items - offset
            ):
                max_scroll = offset
                break
            max_scroll = offset

        self.scroll_offset = max(0, min(self.scroll_offset + steps, max_scroll))

        # If selection is no longer visible, adjust it
        visible_count = self._items_fitting_in_rows(
            self.scroll_offset, self.max_visible_items
        )
        if self.selected_index < self.scroll_offset:
            self.selected_index = self.scroll_offset
        elif self.selected_index >= self.scroll_offset + visible_count:
            self.selected_index = self.scroll_offset + visible_count - 1

    def get_selected_item(self):
        """
        Get the currently selected item.

        Returns:
            SelectableListItem or None if no selection
        """
        if 0 <= self.selected_index < len(self.filtered_items):
            return self.filtered_items[self.selected_index]
        return None

    def get_visible_items(self):
        """
        Get the items in the current scroll window.

        Returns:
            List of (item, is_selected) tuples
        """
        visible = []
        end_index = min(
            self.scroll_offset + self.max_visible_items, len(self.filtered_items)
        )

        for i in range(self.scroll_offset, end_index):
            item = self.filtered_items[i]
            is_selected = i == self.selected_index
            visible.append((item, is_selected))

        return visible

    def calculate_size(self, text_renderer):
        """
        Calculate the bounding box size for layout.

        Width is based on currently visible items (scroll window), so the
        panel shrinks when showing shorter names. Height shrinks when
        filtered items are fewer than the default visible count.

        Args:
            text_renderer: TextRenderer instance for measuring text

        Returns:
            tuple: (width, height) in pixels
        """
        if not text_renderer or not text_renderer.font_atlas:
            return (300.0, 200.0)

        # Count how many items fit in the row budget and the total rows they use
        items_fit = self._items_fitting_in_rows(
            self.scroll_offset, self.default_visible_items
        )
        end_index = min(self.scroll_offset + items_fit, len(self.filtered_items))
        total_rows_used = self._total_rows_for_items(self.scroll_offset, end_index)

        # Use the actual row count (shrinks when fewer items than the budget)
        visible_count = min(total_rows_used, self.default_visible_items)

        # Calculate max width from VISIBLE items only (not all filtered items),
        # so the panel shrinks when showing shorter names
        max_width = 200.0
        for i in range(self.scroll_offset, end_index):
            item = self.filtered_items[i]
            item_width = text_renderer.calculateTextWidth(item.name, self.font_size)
            max_width = max(max_width, item_width)

        # Also measure group headers that are visible (include the "▸ " prefix)
        last_group = None
        for i in range(self.scroll_offset, end_index):
            item = self.filtered_items[i]
            if item.group and item.group != last_group:
                group_width = text_renderer.calculateTextWidth(
                    f"▸ {item.group}", self.font_size
                )
                max_width = max(max_width, group_width)
                last_group = item.group

        # Total height accounts for both items and group headers
        total_height = (visible_count * self.item_height) + (self.padding * 2)
        total_width = max_width + self.padding * 2

        return (total_width, total_height)

    def render(
        self,
        text_renderer,
        x,
        y,
        depth,
        width,
        node_vertex_class,
        draw_fn,
        projection,
        alpha=1.0,
    ):
        """
        Render the selectable list.

        Args:
            text_renderer: TextRenderer instance
            x, y: Position in screen coordinates
            depth: Z-depth for rendering
            width: Width of the list
            node_vertex_class: NodeVertex class for creating geometry
            draw_fn: Function to draw node vertices
            projection: Projection matrix for rendering
            alpha: Overall alpha for the widget (0.0 to 1.0)
        """
        if not text_renderer or not text_renderer.font_atlas:
            return

        if not self.filtered_items:
            return

        current_y = y
        last_group = None
        rows_rendered = 0  # Track total rows (headers + items)

        # Start from scroll offset and render until we hit max_visible_items rows
        i = self.scroll_offset
        while i < len(self.filtered_items) and rows_rendered < self.max_visible_items:
            item = self.filtered_items[i]

            # Draw group header if this is a new group
            if item.group and item.group != last_group:
                # Only render header if we have room for both header and at least one item
                if rows_rendered >= self.max_visible_items:
                    break

                self._render_group_header(
                    text_renderer,
                    item.group,
                    x,
                    current_y,
                    depth,
                    width,
                    node_vertex_class,
                    draw_fn,
                    projection,
                    alpha,
                )
                current_y += self.group_header_height
                last_group = item.group
                rows_rendered += 1

                # Check again after rendering header
                if rows_rendered >= self.max_visible_items:
                    break

            # Draw item
            is_selected = i == self.selected_index
            if is_selected:
                self._render_selected_item(
                    text_renderer,
                    item.name,
                    x,
                    current_y,
                    depth,
                    width,
                    node_vertex_class,
                    draw_fn,
                    projection,
                    alpha,
                )
            else:
                self._render_item(
                    text_renderer,
                    item.name,
                    x,
                    current_y,
                    depth,
                    width,
                    node_vertex_class,
                    draw_fn,
                    projection,
                    alpha,
                )

            current_y += self.item_height
            rows_rendered += 1
            i += 1

    def _render_group_header(
        self,
        text_renderer,
        group_name,
        x,
        y,
        depth,
        width,
        node_vertex_class,
        draw_fn,
        projection,
        alpha,
    ):
        """Render a group header with darker background."""
        # Draw darker background for header
        bg_vertex_data = []
        bg_r = int(self.group_header_bg_color[0] * 255)
        bg_g = int(self.group_header_bg_color[1] * 255)
        bg_b = int(self.group_header_bg_color[2] * 255)
        bg_a = int(self.group_header_bg_color[3] * 255 * alpha)

        bg_vertex_data.append(
            node_vertex_class(
                float(x),
                float(y),
                depth,
                0.0,
                0.0,
                float(width),
                float(self.group_header_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x + width),
                float(y),
                depth,
                float(width),
                0.0,
                float(width),
                float(self.group_header_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x),
                float(y + self.group_header_height),
                depth,
                0.0,
                float(self.group_header_height),
                float(width),
                float(self.group_header_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x + width),
                float(y),
                depth,
                float(width),
                0.0,
                float(width),
                float(self.group_header_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x + width),
                float(y + self.group_header_height),
                depth,
                float(width),
                float(self.group_header_height),
                float(width),
                float(self.group_header_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x),
                float(y + self.group_header_height),
                depth,
                0.0,
                float(self.group_header_height),
                float(width),
                float(self.group_header_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )

        draw_fn(bg_vertex_data, projection, 0.0, (0, 0, 0, 0), generation=-1)

        # Draw group header text centered vertically in the row
        text_x = x + self.padding
        text_y = (
            y
            + (self.group_header_height / 2)
            + (text_renderer.font_atlas.ascender * self.font_size * 0.5)
        )

        text_vertex_data = []
        text_renderer.generateTextVertices(
            f"▸ {group_name}",
            text_x,
            text_y,
            depth,
            self.font_size,
            text_vertex_data,
            node_index=-1.0,
        )
        if text_vertex_data:
            header_color = (
                self.group_header_color[0],
                self.group_header_color[1],
                self.group_header_color[2],
                alpha,
            )
            text_renderer.drawTextVertices(
                text_vertex_data, projection, header_color, disable_depth=True
            )

    def _render_item(
        self,
        text_renderer,
        item_name,
        x,
        y,
        depth,
        width,
        node_vertex_class,
        draw_fn,
        projection,
        alpha,
    ):
        """Render a normal (unselected) item."""
        text_x = x + self.padding * 2
        text_y = (
            y
            + (self.item_height / 2)
            + (text_renderer.font_atlas.ascender * self.font_size * 0.5)
        )

        text_vertex_data = []
        text_renderer.generateTextVertices(
            item_name,
            text_x,
            text_y,
            depth,
            self.font_size,
            text_vertex_data,
            node_index=-1.0,
        )
        if text_vertex_data:
            item_color = (
                self.item_color[0],
                self.item_color[1],
                self.item_color[2],
                alpha,
            )
            text_renderer.drawTextVertices(
                text_vertex_data, projection, item_color, disable_depth=True
            )

    def _render_selected_item(
        self,
        text_renderer,
        item_name,
        x,
        y,
        depth,
        width,
        node_vertex_class,
        draw_fn,
        projection,
        alpha,
    ):
        """Render a selected (highlighted) item."""
        # Draw selection background
        bg_vertex_data = []
        bg_r = int(self.selected_color[0] * 255)
        bg_g = int(self.selected_color[1] * 255)
        bg_b = int(self.selected_color[2] * 255)
        bg_a = int(self.selected_color[3] * 255 * alpha)

        bg_vertex_data.append(
            node_vertex_class(
                float(x),
                float(y),
                depth,
                0.0,
                0.0,
                float(width),
                float(self.item_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x + width),
                float(y),
                depth,
                float(width),
                0.0,
                float(width),
                float(self.item_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x),
                float(y + self.item_height),
                depth,
                0.0,
                float(self.item_height),
                float(width),
                float(self.item_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x + width),
                float(y),
                depth,
                float(width),
                0.0,
                float(width),
                float(self.item_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x + width),
                float(y + self.item_height),
                depth,
                float(width),
                float(self.item_height),
                float(width),
                float(self.item_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        bg_vertex_data.append(
            node_vertex_class(
                float(x),
                float(y + self.item_height),
                depth,
                0.0,
                float(self.item_height),
                float(width),
                float(self.item_height),
                bg_r,
                bg_g,
                bg_b,
                bg_a,
                1.0,
                0,
                0,
                0,
                255,
                0.0,
            )
        )

        draw_fn(bg_vertex_data, projection, 0.0, (0, 0, 0, 0), generation=-1)

        # Draw item text with symbol
        text_x = x + self.padding * 2
        text_y = (
            y
            + (self.item_height / 2)
            + (text_renderer.font_atlas.ascender * self.font_size * 0.5)
        )

        text_vertex_data = []
        text_renderer.generateTextVertices(
            f"► {item_name}",
            text_x,
            text_y,
            depth,
            self.font_size,
            text_vertex_data,
            node_index=-1.0,
        )
        if text_vertex_data:
            # Use white text for selected items
            selected_text_color = (1.0, 1.0, 1.0, alpha)
            text_renderer.drawTextVertices(
                text_vertex_data, projection, selected_text_color, disable_depth=True
            )

    @staticmethod
    def substring_filter(filter_text, item_name):
        """
        Default filter - match anywhere (case-insensitive).

        Args:
            filter_text: Filter string (already lowercased)
            item_name: Item name to check (already lowercased)

        Returns:
            bool: True if item matches filter
        """
        return filter_text in item_name

    @staticmethod
    def prefix_filter(filter_text, item_name):
        """
        Prefix filter - match from start only (case-insensitive).

        Args:
            filter_text: Filter string (already lowercased)
            item_name: Item name to check (already lowercased)

        Returns:
            bool: True if item matches filter
        """
        return item_name.startswith(filter_text)
