#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#

from OpenGL import GL
from pxr import Tf


class NavigationHotkeyPopup:
    """A popup that displays navigation hotkeys while Shift is held.

    Unlike HotkeyPopup, this popup:
    - Displays multi-line content
    - Stays visible as long as Shift is held (no timer)
    - Fades out when explicitly hidden
    - Appears in the top-left corner of the viewport
    """

    def __init__(self, animator):
        self._alpha = animator.addProperty()
        self._alpha.speed = 8.0
        self._alpha.target = 0.0
        self._alpha.current = 0.0

        self._font_size = 18.0
        self._key_font_size = 18.0
        self._popup_margin = 20.0
        self._popup_padding = 16.0
        self._line_spacing = 6.0
        self._corner_radius = 8.0
        self._fade_in_speed = 20.0
        self._fade_out_speed = 8.0

        self._is_showing = False

    @property
    def alpha(self):
        return self._alpha.current

    @property
    def is_visible(self):
        return self._alpha.current > 0.01

    def _get_hotkeys(self):
        from ..noodlesConfig import NoodlesConfig

        return [
            (
                NoodlesConfig.get("shortcutNavigateBackward", "[")
                + "  "
                + NoodlesConfig.get("shortcutNavigateForward", "]"),
                "Navigate backward / forward",
            ),
            ("1-9", "Select link by pin"),
            ("Shift+1-9", "Reverse direction"),
            (NoodlesConfig.get("shortcutRemoveFromGraph", "D"), "Remove from graph"),
            (NoodlesConfig.get("shortcutFrameSelection", "F"), "Frame selection"),
            (NoodlesConfig.get("shortcutFrameAll", "Ctrl+Shift+F"), "Frame all"),
            (
                NoodlesConfig.get("shortcutFrameWithConnections", "Shift+F"),
                "Frame with connections",
            ),
        ]

    def show(self):
        if self._is_showing:
            return
        self._is_showing = True
        self._alpha.current = 0.1  # Start slightly visible for immediate feedback
        self._alpha.target = 1.0
        self._alpha.speed = self._fade_in_speed

    def hide(self):
        if not self._is_showing:
            return
        self._is_showing = False
        self._alpha.target = 0.0
        self._alpha.speed = self._fade_out_speed

    def render(
        self,
        text_renderer,
        node_vertex_class,
        draw_node_vertices_fn,
        projection,
        viewport_width,
        viewport_height,
        max_depth,
    ):
        if not text_renderer or not text_renderer.font_atlas:
            return

        alpha = self._alpha.current
        if alpha < 0.01:
            return

        try:
            depth = max_depth - 0.3
            hotkeys = self._get_hotkeys()

            key_width_max = 0.0
            desc_width_max = 0.0
            line_height = self._font_size * text_renderer.font_atlas.lineHeight

            for key, desc in hotkeys:
                key_width = text_renderer.calculateTextWidth(key, self._key_font_size)
                desc_width = text_renderer.calculateTextWidth(desc, self._font_size)
                key_width_max = max(key_width_max, key_width)
                desc_width_max = max(desc_width_max, desc_width)

            key_desc_spacing = 20.0
            total_text_width = key_width_max + key_desc_spacing + desc_width_max

            total_text_height = (
                len(hotkeys) * line_height + (len(hotkeys) - 1) * self._line_spacing
            )

            popup_x = self._popup_margin
            popup_y = self._popup_margin

            popup_w = total_text_width + self._popup_padding * 2
            popup_h = total_text_height + self._popup_padding * 2

            # Disable depth testing for overlay
            GL.glDisable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_FALSE)

            bg_vertex_data = []
            bg_alpha = int(220 * alpha)
            bg_r, bg_g, bg_b = 20, 22, 28

            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x),
                    float(popup_y),
                    depth,
                    0.0,
                    0.0,
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x + popup_w),
                    float(popup_y),
                    depth,
                    float(popup_w),
                    0.0,
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x),
                    float(popup_y + popup_h),
                    depth,
                    0.0,
                    float(popup_h),
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x + popup_w),
                    float(popup_y),
                    depth,
                    float(popup_w),
                    0.0,
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x + popup_w),
                    float(popup_y + popup_h),
                    depth,
                    float(popup_w),
                    float(popup_h),
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x),
                    float(popup_y + popup_h),
                    depth,
                    0.0,
                    float(popup_h),
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )

            stroke_color = (0.3, 0.4, 0.5, 0.5 * alpha)
            draw_node_vertices_fn(
                bg_vertex_data,
                projection,
                self._corner_radius,
                stroke_color,
                generation=0,
            )

            text_x_key = popup_x + self._popup_padding
            text_x_desc = text_x_key + key_width_max + key_desc_spacing
            text_y = (
                popup_y
                + self._popup_padding
                + text_renderer.font_atlas.ascender * self._font_size
            )

            for key, desc in hotkeys:
                key_vertex_data = []
                text_renderer.generateTextVertices(
                    key,
                    text_x_key,
                    text_y,
                    depth,
                    self._key_font_size,
                    key_vertex_data,
                    node_index=-1.0,
                )
                if key_vertex_data:
                    key_color = (0.6, 0.8, 1.0, alpha)  # Light blue for keys
                    text_renderer.drawTextVertices(
                        key_vertex_data, projection, key_color, disable_depth=True
                    )

                desc_vertex_data = []
                text_renderer.generateTextVertices(
                    desc,
                    text_x_desc,
                    text_y,
                    depth,
                    self._font_size,
                    desc_vertex_data,
                    node_index=-1.0,
                )
                if desc_vertex_data:
                    desc_color = (
                        0.85,
                        0.85,
                        0.85,
                        alpha,
                    )  # Light gray for descriptions
                    text_renderer.drawTextVertices(
                        desc_vertex_data, projection, desc_color, disable_depth=True
                    )

                text_y += line_height + self._line_spacing

            # Re-enable depth testing
            GL.glEnable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_TRUE)

        except Exception as e:
            Tf.Warn(f"Error rendering navigation hotkey popup: {e}")
            import traceback

            traceback.print_exc()
            GL.glEnable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_TRUE)


class HotkeyPopup:
    # Wrap messages so the popup never exceeds this fraction of viewport width.
    # Word-wrapping kicks in inside render() once the message overflows.
    _MAX_WIDTH_FRACTION = 0.4
    _MIN_WRAP_WIDTH = 200.0

    def __init__(self, animator):
        self._alpha = animator.addProperty()
        self._alpha.speed = 8.0
        self._alpha.target = 0.0
        self._alpha.current = 0.0

        self._message = ""
        self._timer_id = None
        self._position = None

        self._font_size = 24.0
        self._popup_margin = 20.0
        self._popup_padding = 12.0
        self._corner_radius = 6.0
        self._fade_in_speed = 25.0  # Fast fade in
        self._fade_out_speed = 6.0  # Slower fade out
        self._display_duration_ms = 400  # 400ms display time

        self._parent_widget = None

    def set_parent_widget(self, widget):
        self._parent_widget = widget

    @property
    def alpha(self):
        return self._alpha.current

    @property
    def is_visible(self):
        return self._alpha.current > 0.01

    def show(self, message, cursor_pos, duration_ms=None):
        self._message = message
        self._alpha.current = 0.0
        self._alpha.target = 1.0
        self._alpha.speed = self._fade_in_speed

        # Capture the cursor position when the popup is first shown
        self._position = (cursor_pos.x(), cursor_pos.y())

        if self._timer_id is not None and self._parent_widget is not None:
            self._parent_widget.killTimer(self._timer_id)
            self._timer_id = None

        if self._parent_widget is not None:
            timer_duration = (
                duration_ms if duration_ms is not None else self._display_duration_ms
            )
            self._timer_id = self._parent_widget.startTimer(timer_duration)

    @staticmethod
    def _wrap_message(message, text_renderer, font_size, max_width):
        """Word-wrap *message* so no line exceeds *max_width* pixels.

        Honors explicit ``\\n`` separators in the message; within each
        explicit line, words are packed greedily and a word too wide to fit
        on its own line is still emitted on a line by itself rather than
        being silently dropped.
        """
        wrapped = []
        for explicit_line in message.splitlines() or [message]:
            words = explicit_line.split()
            if not words:
                wrapped.append("")
                continue
            current = words[0]
            for word in words[1:]:
                candidate = current + " " + word
                if text_renderer.calculateTextWidth(candidate, font_size) <= max_width:
                    current = candidate
                else:
                    wrapped.append(current)
                    current = word
            wrapped.append(current)
        return wrapped

    def handle_timer_event(self, timer_id):
        if timer_id == self._timer_id:
            if self._parent_widget is not None:
                self._parent_widget.killTimer(self._timer_id)
            self._timer_id = None
            self._alpha.target = 0.0
            self._alpha.speed = self._fade_out_speed
            return True
        return False

    def render(
        self,
        text_renderer,
        node_vertex_class,
        draw_node_vertices_fn,
        projection,
        viewport_width,
        viewport_height,
        max_depth,
    ):
        if not text_renderer or not text_renderer.font_atlas:
            return

        alpha = self._alpha.current
        if alpha < 0.01:
            return

        try:
            depth = max_depth - 0.3  # Very front, above minimap

            # Wrap the message so a single long line never overflows the viewport.
            line_height = self._font_size * text_renderer.font_atlas.lineHeight
            max_text_width = max(
                self._MIN_WRAP_WIDTH,
                viewport_width * self._MAX_WIDTH_FRACTION,
            )
            lines = self._wrap_message(
                self._message, text_renderer, self._font_size, max_text_width
            )
            if not lines:
                return
            text_width = max(
                text_renderer.calculateTextWidth(line, self._font_size)
                for line in lines
            )
            text_height = line_height * len(lines)

            # Use the fixed position captured when popup was shown
            if self._position is None:
                return
            cursor_x, cursor_y = self._position
            popup_x = cursor_x + self._popup_margin
            popup_y = (
                cursor_y - self._popup_margin - text_height - self._popup_padding * 2
            )

            popup_w = text_width + self._popup_padding * 2
            popup_h = text_height + self._popup_padding * 2

            if popup_x + popup_w > viewport_width - 10:
                popup_x = cursor_x - popup_w - self._popup_margin
            if popup_y < 10:
                popup_y = cursor_y + self._popup_margin

            # Clamp to window bounds
            popup_x = max(10, min(popup_x, viewport_width - popup_w - 10))
            popup_y = max(10, min(popup_y, viewport_height - popup_h - 10))

            # Disable depth testing for overlay
            GL.glDisable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_FALSE)

            bg_vertex_data = []
            bg_alpha = int(200 * alpha)
            bg_r, bg_g, bg_b = 25, 25, 30

            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x),
                    float(popup_y),
                    depth,
                    0.0,
                    0.0,
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x + popup_w),
                    float(popup_y),
                    depth,
                    float(popup_w),
                    0.0,
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x),
                    float(popup_y + popup_h),
                    depth,
                    0.0,
                    float(popup_h),
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x + popup_w),
                    float(popup_y),
                    depth,
                    float(popup_w),
                    0.0,
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x + popup_w),
                    float(popup_y + popup_h),
                    depth,
                    float(popup_w),
                    float(popup_h),
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x),
                    float(popup_y + popup_h),
                    depth,
                    0.0,
                    float(popup_h),
                    float(popup_w),
                    float(popup_h),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_alpha,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )

            stroke_color = (0.4, 0.4, 0.5, 0.6 * alpha)
            draw_node_vertices_fn(
                bg_vertex_data,
                projection,
                self._corner_radius,
                stroke_color,
                generation=0,
            )

            # Draw text — one generateTextVertices call per wrapped line.
            text_vertex_data = []
            text_x = popup_x + self._popup_padding
            baseline_y = (
                popup_y
                + self._popup_padding
                + text_renderer.font_atlas.ascender * self._font_size
            )

            for i, line in enumerate(lines):
                text_renderer.generateTextVertices(
                    line,
                    text_x,
                    baseline_y + i * line_height,
                    depth,
                    self._font_size,
                    text_vertex_data,
                    node_index=-1.0,
                )

            if text_vertex_data:
                text_color = (1.0, 1.0, 1.0, alpha)
                text_renderer.drawTextVertices(
                    text_vertex_data, projection, text_color, disable_depth=True
                )

            # Re-enable depth testing
            GL.glEnable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_TRUE)

        except Exception as e:
            Tf.Warn(f"Error rendering hotkey popup: {e}")
            import traceback

            traceback.print_exc()
            GL.glEnable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_TRUE)
