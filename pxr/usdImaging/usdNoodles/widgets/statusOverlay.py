#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#

from pxr import Tf

from ..noodlesConfig import NoodlesConfig


class StatusOverlay:
    def __init__(self):
        self._margin = 16.0
        self._name_font_size = 16.0
        self._type_font_size = 12.0
        self._bg_corner_radius = 3.0
        self._bg_stroke_width = 2.0

    def render(
        self,
        node,
        text_renderer,
        node_vertex_class,
        draw_node_vertices_fn,
        projection,
        screen_height,
        max_depth,
    ):
        if not node or not text_renderer or not text_renderer.font_atlas:
            return

        # Get config values
        name_font_size = NoodlesConfig.get("statusOverlayNameSize", 16.0)
        type_font_size = NoodlesConfig.get("statusOverlayTypeSize", 12.0)

        node_name = node.name
        node_type = node.type

        try:
            status_vertex_data = []
            max_cursor_x = 0.0
            min_cursor_y = screen_height
            cursor_x = self._margin
            cursor_y = screen_height - self._margin
            depth = max_depth

            cursor_x, _ = text_renderer.generateTextVertices(
                node_name,
                cursor_x,
                cursor_y,
                depth,
                name_font_size,
                status_vertex_data,
                node_index=-1.0,
            )
            max_cursor_x = max(max_cursor_x, cursor_x)

            cursor_y -= text_renderer.font_atlas.lineHeight * name_font_size
            cursor_x = self._margin

            cursor_x, _ = text_renderer.generateTextVertices(
                node_type,
                cursor_x,
                cursor_y,
                depth,
                type_font_size,
                status_vertex_data,
                node_index=-1.0,
            )
            max_cursor_x = max(max_cursor_x, cursor_x)

            cursor_y -= text_renderer.font_atlas.lineHeight * type_font_size
            min_cursor_y = min(min_cursor_y, cursor_y)

            if not status_vertex_data:
                return

            background_vertex_data = self._create_background(
                node_vertex_class, max_cursor_x, min_cursor_y, screen_height, depth
            )

            stroke_color = (1.0, 1.0, 1.0, 0.196)
            draw_node_vertices_fn(
                background_vertex_data,
                projection,
                self._bg_corner_radius,
                stroke_color,
                generation=0,
            )

            text_renderer.drawTextVertices(
                status_vertex_data,
                projection,
                color=(1.0, 1.0, 1.0, 1.0),
                disable_depth=True,
            )

        except Exception as e:
            Tf.Warn(f"Error rendering status overlay: {e}")

    def _create_background(
        self, node_vertex_class, max_cursor_x, min_cursor_y, screen_height, depth
    ):
        background_vertex_data = []

        w = max_cursor_x + self._margin
        h = screen_height - min_cursor_y - self._margin * 0.5
        x0 = self._margin * 0.5
        y0 = min_cursor_y
        x1 = x0 + w
        y1 = y0 + h

        bg_high = 30
        bg_low = 10
        bg_alpha = 200
        shadow = 0.3
        bg_high_shadow = int(bg_high * shadow)
        bg_low_shadow = int(bg_low * shadow)

        # First triangle
        background_vertex_data.append(
            node_vertex_class(
                float(x0),
                float(y0),
                depth,
                0.0,
                0.0,
                float(w),
                float(h),
                bg_high,
                bg_low,
                bg_low,
                bg_alpha,
                self._bg_stroke_width,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        background_vertex_data.append(
            node_vertex_class(
                float(x1),
                float(y0),
                depth,
                float(w),
                0.0,
                float(w),
                float(h),
                bg_high,
                bg_low,
                bg_high,
                bg_alpha,
                self._bg_stroke_width,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        background_vertex_data.append(
            node_vertex_class(
                float(x0),
                float(y1),
                depth,
                0.0,
                float(h),
                float(w),
                float(h),
                bg_low_shadow,
                bg_high_shadow,
                bg_low_shadow,
                bg_alpha,
                self._bg_stroke_width,
                0,
                0,
                0,
                255,
                0.0,
            )
        )

        # Second triangle
        background_vertex_data.append(
            node_vertex_class(
                float(x1),
                float(y0),
                depth,
                float(w),
                0.0,
                float(w),
                float(h),
                bg_high,
                bg_low,
                bg_high,
                bg_alpha,
                self._bg_stroke_width,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        background_vertex_data.append(
            node_vertex_class(
                float(x1),
                float(y1),
                depth,
                float(w),
                float(h),
                float(w),
                float(h),
                bg_low_shadow,
                bg_low_shadow,
                bg_high_shadow,
                bg_alpha,
                self._bg_stroke_width,
                0,
                0,
                0,
                255,
                0.0,
            )
        )
        background_vertex_data.append(
            node_vertex_class(
                float(x0),
                float(y1),
                depth,
                0.0,
                float(h),
                float(w),
                float(h),
                bg_low_shadow,
                bg_high_shadow,
                bg_low_shadow,
                bg_alpha,
                self._bg_stroke_width,
                0,
                0,
                0,
                255,
                0.0,
            )
        )

        return background_vertex_data
