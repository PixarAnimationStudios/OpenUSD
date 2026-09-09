#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#

"""
Node Creation Hotbox Widget.

A searchable popup widget for creating USD node graph prims, combining a text
input filter with a selectable list of prim types.
"""

from OpenGL import GL
from pxr import Tf

from .selectableList import SelectableList, SelectableListItem
from .textInputWidget import TextInputWidget
from .widgetBase import OverlayWidget


class NodeTypeGroup:
    """
    A group of node types (e.g., "Prims", "Shaders").

    Attributes:
        name: Group display name
        types: List of node type strings
    """

    def __init__(self, name, types):
        self.name = name
        self.types = types


class NodeCreationHotbox(OverlayWidget):
    """
    Searchable node creation popup (hotbox).

    Combines TextInputWidget and SelectableList to provide a quick way to
    search and create node prims in the graph.

    Features:
    - Text input filter at top
    - Scrollable filtered list below
    - Keyboard navigation
    - Automatic positioning at cursor
    - Callback on node creation
    """

    def __init__(self, animator):
        """
        Initialize the hotbox widget.

        Args:
            animator: Animator instance for managing alpha animation
        """
        super().__init__(animator)

        self._text_input = TextInputWidget()
        self._selectable_list = SelectableList(max_visible_items=20)

        # Position where hotbox was opened (screen coordinates)
        self._position = (0, 0)

        # Node groups to display
        self._node_groups = []

        # Callback when node is created
        self.on_create = None

        # Visual configuration
        self._padding = 12.0
        self._corner_radius = 6.0
        self._background_color = (0.1, 0.1, 0.12, 0.95)
        self._max_visible_items = 20
        self._default_visible_items = 20  # Track default for height calculation

    def configure(self, node_groups, max_visible=20):
        """
        Configure the available node types.

        Args:
            node_groups: List of NodeTypeGroup objects
            max_visible: Maximum number of items to show in list
        """
        self._node_groups = node_groups
        self._max_visible_items = max_visible
        self._default_visible_items = max_visible
        self._selectable_list.max_visible_items = max_visible

        # Build flat list of items from groups
        items = []
        for group in node_groups:
            for node_type in group.types:
                item = SelectableListItem(node_type, group.name, node_type)
                items.append(item)

        self._selectable_list.set_items(items)

    def configure_from_libraries(self, node_items):
        """
        Configure the hotbox from library node items.

        This is the new library-aware configuration method that stores
        library references alongside node identifiers.

        Args:
            node_items: List of dicts, each containing:
                - 'name': Display name
                - 'family': Family/group name
                - 'identifier': Node type identifier
                - 'library': Reference to the NodeLibrary for creation
        """
        # Build flat list of items with library data embedded
        items = []
        for node_item in node_items:
            # Create item data dict with library reference
            item_data = {
                "identifier": node_item["identifier"],
                "library": node_item["library"],
            }

            # Create SelectableListItem with library data
            item = SelectableListItem(
                node_item["name"],  # Display name
                node_item["family"],  # Group name
                item_data,  # Data: dict with library + identifier
            )
            items.append(item)

        self._selectable_list.set_items(items)

    def show(self, screen_pos=None):
        """
        Show the hotbox at the specified position.

        Args:
            screen_pos: QPoint or tuple (x, y) for screen position, or None for center
        """
        if screen_pos is not None:
            if hasattr(screen_pos, "x"):
                self._position = (screen_pos.x(), screen_pos.y())
            else:
                self._position = screen_pos
        else:
            self._position = (0, 0)

        # Reset state
        self._text_input.clear()
        self._selectable_list.set_filter("")

        # Show with animation
        super().show()

    def handle_scroll(self, steps):
        """
        Handle mouse wheel scroll events.

        Args:
            steps: Number of items to scroll (positive = down, negative = up)
        """
        self._selectable_list.scroll(steps)

    def handle_mouse_press(self, screen_pos):
        """
        Handle mouse press events.

        Args:
            screen_pos: QPoint representing screen position

        Returns:
            bool: True if the click was handled (within hotbox), False otherwise
        """
        if not self.is_visible:
            return False

        # Check if click is within hotbox bounds
        if not hasattr(self, "_bounds") or not self._bounds:
            return False

        x, y, width, height = self._bounds
        click_x = screen_pos.x()
        click_y = screen_pos.y()

        if not (x <= click_x <= x + width and y <= click_y <= y + height):
            # Click outside hotbox - hide it
            self.hide()
            return False

        # Click is within hotbox - check if it's on a list item
        # Get the input widget height to offset the list position
        input_height = self._text_input.calculate_size(None)[1]
        list_y = y + self._padding + input_height + self._padding

        # Check if click is within the list area
        if click_y >= list_y:
            # Find which item was clicked by iterating through visible items
            # and accounting for group headers
            actual_index = self._find_item_at_y_position(click_y - list_y)

            if actual_index is not None and 0 <= actual_index < len(
                self._selectable_list.filtered_items
            ):
                # Valid item clicked - select it and create node
                selected_item = self._selectable_list.filtered_items[actual_index]
                if self.on_create:
                    self.on_create(selected_item.data, self._position)
                self.hide()
                return True

        return True  # Consumed the click but didn't trigger creation

    def handle_mouse_move(self, screen_pos):
        """
        Handle mouse move events for hover selection.

        Args:
            screen_pos: QPoint representing screen position

        Returns:
            bool: True if the move was handled (within hotbox), False otherwise
        """
        if not self.is_visible:
            return False

        # Check if mouse is within hotbox bounds
        if not hasattr(self, "_bounds") or not self._bounds:
            return False

        x, y, width, height = self._bounds
        mouse_x = screen_pos.x()
        mouse_y = screen_pos.y()

        if not (x <= mouse_x <= x + width and y <= mouse_y <= y + height):
            return False

        # Mouse is within hotbox - check if it's over a list item
        input_height = self._text_input.calculate_size(None)[1]
        list_y = y + self._padding + input_height + self._padding

        if mouse_y >= list_y:
            # Find which item is hovered by iterating through visible items
            # and accounting for group headers
            actual_index = self._find_item_at_y_position(mouse_y - list_y)

            if actual_index is not None and 0 <= actual_index < len(
                self._selectable_list.filtered_items
            ):
                # Valid item hovered - update selection
                if self._selectable_list.selected_index != actual_index:
                    self._selectable_list.selected_index = actual_index
                    return True  # Need to redraw

        return False

    def _find_item_at_y_position(self, relative_y):
        """
        Find which item is at a given Y position, accounting for group headers.

        Args:
            relative_y: Y position relative to the list's top (after list padding)

        Returns:
            int: Index of the item in filtered_items, or None if not found
        """
        # Start after the list's top padding
        current_y = self._selectable_list.padding

        if relative_y < current_y:
            return None

        last_group = None
        end_index = min(
            self._selectable_list.scroll_offset
            + self._selectable_list.max_visible_items,
            len(self._selectable_list.filtered_items),
        )

        for i in range(self._selectable_list.scroll_offset, end_index):
            if i >= len(self._selectable_list.filtered_items):
                break

            item = self._selectable_list.filtered_items[i]

            # Check if we need to draw a group header
            if item.group and item.group != last_group:
                group_header_end = current_y + self._selectable_list.group_header_height
                # Skip over the group header (we don't select headers)
                if relative_y < group_header_end:
                    return None
                current_y = group_header_end
                last_group = item.group

            # Check if the click/hover is on this item
            item_end = current_y + self._selectable_list.item_height
            if current_y <= relative_y < item_end:
                return i

            current_y = item_end

        return None

    def handle_key(self, event):
        """
        Handle keyboard input.

        Args:
            event: QKeyEvent from Qt

        Returns:
            bool: True if the key was handled, False otherwise
        """
        from pxr.Usdviewq.qt import QtCore

        key = event.key()

        # Tab - toggle off (hide)
        if key == QtCore.Qt.Key_Tab:
            self.hide()
            return True

        # Escape - hide
        elif key == QtCore.Qt.Key_Escape:
            self.hide()
            return True

        # Enter - create selected node and hide
        elif key == QtCore.Qt.Key_Return or key == QtCore.Qt.Key_Enter:
            selected_item = self._selectable_list.get_selected_item()
            if selected_item and self.on_create:
                self.on_create(selected_item.data, self._position)
            self.hide()
            return True

        # Up/Down - navigate list
        elif key == QtCore.Qt.Key_Up:
            self._selectable_list.move_selection(-1)
            if self._parent_widget:
                self._parent_widget.update()
            return True

        elif key == QtCore.Qt.Key_Down:
            self._selectable_list.move_selection(1)
            if self._parent_widget:
                self._parent_widget.update()
            return True

        # Other keys - delegate to text input
        else:
            old_text = self._text_input.text
            handled = self._text_input.handle_key(event)

            # If text changed, update filter
            if handled and self._text_input.text != old_text:
                self._selectable_list.set_filter(self._text_input.text)
                if self._parent_widget:
                    self._parent_widget.update()

            return handled

    def render(
        self,
        text_renderer,
        node_vertex_class,
        draw_fn,
        projection,
        viewport_width,
        viewport_height,
        max_depth,
    ):
        """
        Render the hotbox widget.

        Args:
            text_renderer: TextRenderer instance
            node_vertex_class: NodeVertex class for creating geometry
            draw_fn: Function to draw node vertices (usually _drawNodeVertices)
            projection: Projection matrix for screen-space rendering
            viewport_width: Viewport width in pixels
            viewport_height: Viewport height in pixels
            max_depth: Maximum depth value for z-ordering
        """
        if not text_renderer or not text_renderer.font_atlas:
            return

        alpha = self.alpha
        if alpha < 0.01:
            return

        try:
            depth = max_depth - 0.2  # In front of other overlays

            # Disable depth testing for overlay
            GL.glDisable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_FALSE)

            # Calculate sizes
            input_width, input_height = self._text_input.calculate_size(text_renderer)
            list_width, list_height = self._selectable_list.calculate_size(
                text_renderer
            )

            # Use the wider of the two
            popup_width = max(input_width, list_width, 250.0) + self._padding * 2
            popup_height = input_height + list_height + self._padding * 3

            # Calculate position (clamped to viewport)
            popup_x = self._position[0]
            popup_y = self._position[1]

            # Clamp to viewport bounds
            if popup_x + popup_width > viewport_width - 10:
                popup_x = viewport_width - popup_width - 10
            if popup_y + popup_height > viewport_height - 10:
                popup_y = viewport_height - popup_height - 10
            popup_x = max(10, popup_x)
            popup_y = max(10, popup_y)

            # Store bounds for mouse event handling
            self._bounds = (popup_x, popup_y, popup_width, popup_height)

            # Draw main background
            bg_vertex_data = []
            bg_r = int(self._background_color[0] * 255)
            bg_g = int(self._background_color[1] * 255)
            bg_b = int(self._background_color[2] * 255)
            bg_a = int(self._background_color[3] * 255 * alpha)

            # NodeVertex signature: x, y, z, u, v, w, h, r, g, b, a, innerStroke, sr, sg, sb, sa, selected
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x),
                    float(popup_y),
                    depth,
                    0.0,
                    0.0,
                    float(popup_width),
                    float(popup_height),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_a,
                    2.0,
                    0,
                    0,
                    0,
                    255,  # selected color (sr, sg, sb, sa)
                    0.0,  # selected flag
                )
            )
            bg_vertex_data.append(
                node_vertex_class(
                    float(popup_x + popup_width),
                    float(popup_y),
                    depth,
                    float(popup_width),
                    0.0,
                    float(popup_width),
                    float(popup_height),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_a,
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
                    float(popup_y + popup_height),
                    depth,
                    0.0,
                    float(popup_height),
                    float(popup_width),
                    float(popup_height),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_a,
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
                    float(popup_x + popup_width),
                    float(popup_y),
                    depth,
                    float(popup_width),
                    0.0,
                    float(popup_width),
                    float(popup_height),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_a,
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
                    float(popup_x + popup_width),
                    float(popup_y + popup_height),
                    depth,
                    float(popup_width),
                    float(popup_height),
                    float(popup_width),
                    float(popup_height),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_a,
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
                    float(popup_y + popup_height),
                    depth,
                    0.0,
                    float(popup_height),
                    float(popup_width),
                    float(popup_height),
                    bg_r,
                    bg_g,
                    bg_b,
                    bg_a,
                    2.0,
                    0,
                    0,
                    0,
                    255,
                    0.0,
                )
            )

            stroke_color = (0.3, 0.4, 0.5, 0.6 * alpha)
            draw_fn(
                bg_vertex_data,
                projection,
                self._corner_radius,
                stroke_color,
                generation=-1,
            )

            # Render text input at top
            input_x = popup_x + self._padding
            input_y = popup_y + self._padding
            input_render_width = popup_width - self._padding * 2

            self._text_input.render(
                text_renderer,
                input_x,
                input_y,
                depth,
                input_render_width,
                node_vertex_class,
                draw_fn,
                projection,
                alpha,
            )

            # Render selectable list below
            list_x = popup_x + self._padding
            list_y = input_y + input_height + self._padding
            list_render_width = popup_width - self._padding * 2

            self._selectable_list.render(
                text_renderer,
                list_x,
                list_y,
                depth,
                list_render_width,
                node_vertex_class,
                draw_fn,
                projection,
                alpha,
            )

            # Re-enable depth testing
            GL.glEnable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_TRUE)

        except Exception as e:
            Tf.Warn(f"Error rendering node creation hotbox: {e}")
            import traceback

            traceback.print_exc()
            GL.glEnable(GL.GL_DEPTH_TEST)
            GL.glDepthMask(GL.GL_TRUE)
