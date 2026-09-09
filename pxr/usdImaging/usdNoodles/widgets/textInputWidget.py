#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#

"""
Reusable single-line text input widget with cursor support.

Provides a text input field that can be embedded in other widgets,
with support for cursor positioning, text editing, and placeholders.
"""

import time

from pxr.Usdviewq.qt import QtCore


class TextInputWidget:
    """
    Reusable single-line text input with cursor.

    Features:
    - Text editing with cursor position
    - Keyboard input handling (alphanumeric, backspace, delete)
    - Cursor movement (left/right, home/end)
    - Placeholder text when empty
    - Blinking cursor animation
    """

    def __init__(self):
        """Initialize the text input widget."""
        self.text = ""
        self.cursor_position = 0
        self.placeholder = "Type to filter..."
        self.font_size = 16.0
        self.padding = 8.0
        self.background_color = (0.15, 0.15, 0.18, 1.0)
        self.text_color = (0.95, 0.95, 0.95, 1.0)
        self.placeholder_color = (0.5, 0.5, 0.55, 1.0)
        self.cursor_color = (0.8, 0.8, 0.9, 1.0)
        self.cursor_blink_rate = 0.5
        self._last_cursor_blink = time.time()
        self._cursor_visible = True

    def handle_key(self, event):
        """
        Handle keyboard input for text editing.

        Args:
            event: QKeyEvent from Qt

        Returns:
            bool: True if the key was handled, False otherwise
        """
        key = event.key()

        # Handle special keys
        if key == QtCore.Qt.Key_Backspace:
            if self.cursor_position > 0:
                self.text = (
                    self.text[: self.cursor_position - 1]
                    + self.text[self.cursor_position :]
                )
                self.cursor_position -= 1
                self._reset_cursor_blink()
            return True

        elif key == QtCore.Qt.Key_Delete:
            if self.cursor_position < len(self.text):
                self.text = (
                    self.text[: self.cursor_position]
                    + self.text[self.cursor_position + 1 :]
                )
                self._reset_cursor_blink()
            return True

        elif key == QtCore.Qt.Key_Left:
            if self.cursor_position > 0:
                self.cursor_position -= 1
                self._reset_cursor_blink()
            return True

        elif key == QtCore.Qt.Key_Right:
            if self.cursor_position < len(self.text):
                self.cursor_position += 1
                self._reset_cursor_blink()
            return True

        elif key == QtCore.Qt.Key_Home:
            self.cursor_position = 0
            self._reset_cursor_blink()
            return True

        elif key == QtCore.Qt.Key_End:
            self.cursor_position = len(self.text)
            self._reset_cursor_blink()
            return True

        # Handle text input
        text = event.text()
        if (
            text
            and text.isprintable()
            and not event.modifiers() & QtCore.Qt.ControlModifier
        ):
            self.text = (
                self.text[: self.cursor_position]
                + text
                + self.text[self.cursor_position :]
            )
            self.cursor_position += len(text)
            self._reset_cursor_blink()
            return True

        return False

    def clear(self):
        """Reset text and cursor to empty state."""
        self.text = ""
        self.cursor_position = 0
        self._reset_cursor_blink()

    def _reset_cursor_blink(self):
        """Reset cursor blink timer to make cursor visible."""
        self._last_cursor_blink = time.time()
        self._cursor_visible = True

    def _update_cursor_blink(self):
        """Update cursor blinking state."""
        current_time = time.time()
        if current_time - self._last_cursor_blink > self.cursor_blink_rate:
            self._cursor_visible = not self._cursor_visible
            self._last_cursor_blink = current_time

    def calculate_size(self, text_renderer):
        """
        Calculate the bounding box size for layout.

        Args:
            text_renderer: TextRenderer instance for measuring text

        Returns:
            tuple: (width, height) in pixels
        """
        if not text_renderer or not text_renderer.font_atlas:
            return (200.0, 30.0)

        # Calculate text width (use placeholder if empty)
        display_text = self.text if self.text else self.placeholder
        text_width = text_renderer.calculateTextWidth(display_text, self.font_size)
        text_height = self.font_size * text_renderer.font_atlas.lineHeight

        width = text_width + self.padding * 2
        height = text_height + self.padding * 2

        return (width, height)

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
        Render the text input widget.

        Args:
            text_renderer: TextRenderer instance
            x, y: Position in screen coordinates
            depth: Z-depth for rendering
            width: Width of the input field
            node_vertex_class: NodeVertex class for creating geometry
            draw_fn: Function to draw node vertices (usually _drawNodeVertices)
            projection: Projection matrix for rendering
            alpha: Overall alpha for the widget (0.0 to 1.0)
        """
        if not text_renderer or not text_renderer.font_atlas:
            return

        # Update cursor blink
        self._update_cursor_blink()

        # Calculate height
        text_height = self.font_size * text_renderer.font_atlas.lineHeight
        height = text_height + self.padding * 2

        # Draw background - NodeVertex signature:
        # (x, y, z, u, v, w, h, r, g, b, a, innerStroke, sr, sg, sb, sa, selected)
        bg_vertex_data = []
        bg_r = int(self.background_color[0] * 255)
        bg_g = int(self.background_color[1] * 255)
        bg_b = int(self.background_color[2] * 255)
        bg_a = int(self.background_color[3] * 255 * alpha)

        bg_vertex_data.append(
            node_vertex_class(
                float(x),
                float(y),
                depth,
                0.0,
                0.0,
                float(width),
                float(height),
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
                float(height),
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
                float(y + height),
                depth,
                0.0,
                float(height),
                float(width),
                float(height),
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
                float(height),
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
                float(y + height),
                depth,
                float(width),
                float(height),
                float(width),
                float(height),
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
                float(y + height),
                depth,
                0.0,
                float(height),
                float(width),
                float(height),
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

        stroke_color = (0.3, 0.3, 0.35, 0.6 * alpha)
        draw_fn(bg_vertex_data, projection, 2.0, stroke_color, generation=-1)

        # Draw text (or placeholder)
        text_x = x + self.padding
        text_y = y + self.padding + text_renderer.font_atlas.ascender * self.font_size

        if self.text:
            # Draw actual text
            text_vertex_data = []
            text_renderer.generateTextVertices(
                self.text,
                text_x,
                text_y,
                depth,
                self.font_size,
                text_vertex_data,
                node_index=-1.0,
            )
            if text_vertex_data:
                text_color = (
                    self.text_color[0],
                    self.text_color[1],
                    self.text_color[2],
                    alpha,
                )
                text_renderer.drawTextVertices(
                    text_vertex_data, projection, text_color, disable_depth=True
                )

            # Draw cursor if visible
            if self._cursor_visible:
                # Calculate cursor X position
                cursor_text = self.text[: self.cursor_position]
                cursor_x = text_x + text_renderer.calculateTextWidth(
                    cursor_text, self.font_size
                )

                cursor_y1 = y + self.padding
                cursor_y2 = y + height - self.padding
                cursor_width = 2.0
                cursor_height = cursor_y2 - cursor_y1

                cursor_vertex_data = []
                cursor_r = int(self.cursor_color[0] * 255)
                cursor_g = int(self.cursor_color[1] * 255)
                cursor_b = int(self.cursor_color[2] * 255)
                cursor_a = int(self.cursor_color[3] * 255 * alpha)

                # NodeVertex signature: x, y, z, u, v, w, h, r, g, b, a, innerStroke, sr, sg, sb, sa, selected
                cursor_vertex_data.append(
                    node_vertex_class(
                        float(cursor_x),
                        float(cursor_y1),
                        depth,
                        0.0,
                        0.0,
                        float(cursor_width),
                        float(cursor_height),
                        cursor_r,
                        cursor_g,
                        cursor_b,
                        cursor_a,
                        1.0,
                        0,
                        0,
                        0,
                        255,
                        0.0,
                    )
                )
                cursor_vertex_data.append(
                    node_vertex_class(
                        float(cursor_x + cursor_width),
                        float(cursor_y1),
                        depth,
                        float(cursor_width),
                        0.0,
                        float(cursor_width),
                        float(cursor_height),
                        cursor_r,
                        cursor_g,
                        cursor_b,
                        cursor_a,
                        1.0,
                        0,
                        0,
                        0,
                        255,
                        0.0,
                    )
                )
                cursor_vertex_data.append(
                    node_vertex_class(
                        float(cursor_x),
                        float(cursor_y2),
                        depth,
                        0.0,
                        float(cursor_height),
                        float(cursor_width),
                        float(cursor_height),
                        cursor_r,
                        cursor_g,
                        cursor_b,
                        cursor_a,
                        1.0,
                        0,
                        0,
                        0,
                        255,
                        0.0,
                    )
                )
                cursor_vertex_data.append(
                    node_vertex_class(
                        float(cursor_x + cursor_width),
                        float(cursor_y1),
                        depth,
                        float(cursor_width),
                        0.0,
                        float(cursor_width),
                        float(cursor_height),
                        cursor_r,
                        cursor_g,
                        cursor_b,
                        cursor_a,
                        1.0,
                        0,
                        0,
                        0,
                        255,
                        0.0,
                    )
                )
                cursor_vertex_data.append(
                    node_vertex_class(
                        float(cursor_x + cursor_width),
                        float(cursor_y2),
                        depth,
                        float(cursor_width),
                        float(cursor_height),
                        float(cursor_width),
                        float(cursor_height),
                        cursor_r,
                        cursor_g,
                        cursor_b,
                        cursor_a,
                        1.0,
                        0,
                        0,
                        0,
                        255,
                        0.0,
                    )
                )
                cursor_vertex_data.append(
                    node_vertex_class(
                        float(cursor_x),
                        float(cursor_y2),
                        depth,
                        0.0,
                        float(cursor_height),
                        float(cursor_width),
                        float(cursor_height),
                        cursor_r,
                        cursor_g,
                        cursor_b,
                        cursor_a,
                        1.0,
                        0,
                        0,
                        0,
                        255,
                        0.0,
                    )
                )

                draw_fn(
                    cursor_vertex_data, projection, 0.0, (0, 0, 0, 0), generation=-1
                )
        else:
            # Draw placeholder text
            text_vertex_data = []
            text_renderer.generateTextVertices(
                self.placeholder,
                text_x,
                text_y,
                depth,
                self.font_size,
                text_vertex_data,
                node_index=-1.0,
            )
            if text_vertex_data:
                placeholder_color = (
                    self.placeholder_color[0],
                    self.placeholder_color[1],
                    self.placeholder_color[2],
                    alpha,
                )
                text_renderer.drawTextVertices(
                    text_vertex_data, projection, placeholder_color, disable_depth=True
                )
