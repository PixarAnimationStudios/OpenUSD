#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

#

"""
Base class for screen-space overlay widgets.

Provides common functionality for widgets that render as screen-space overlays,
including alpha animation for show/hide transitions.
"""


def make_node_vertex(
    node_vertex_class,
    x,
    y,
    z,
    u,
    v,
    w,
    h,
    r,
    g,
    b,
    a,
    inner_stroke=0.0,
    sr=0,
    sg=0,
    sb=0,
    sa=255,
    selected=0.0,
):
    """
    Create a NodeVertex instance with the correct signature.

    This wrapper ensures compatibility with the C++ NodeVertex class regardless
    of signature changes. All coordinates should be floats and colors should
    be uint8 (0-255).

    Args:
        node_vertex_class: The NodeVertex class to instantiate
        x, y, z: Position coordinates (float)
        u, v: UV coordinates (float)
        w, h: Size (float)
        r, g, b, a: Fill color (uint8, 0-255)
        inner_stroke: Inner stroke width (float)
        sr, sg, sb, sa: Selected color (uint8, 0-255)
        selected: Selection flag (float, 0.0 or 1.0)

    Returns:
        NodeVertex instance
    """
    return node_vertex_class(
        float(x),
        float(y),
        float(z),
        float(u),
        float(v),
        float(w),
        float(h),
        int(r),
        int(g),
        int(b),
        int(a),
        float(inner_stroke),
        int(sr),
        int(sg),
        int(sb),
        int(sa),
        float(selected),
    )


class OverlayWidget:
    """
    Minimal base class for screen-space overlay widgets.

    Provides:
    - Alpha animation property for fade in/out
    - Common visibility logic
    - show()/hide() methods
    """

    def __init__(self, animator):
        """
        Initialize the overlay widget with animation support.

        Args:
            animator: Animator instance for managing alpha animation
        """
        self._alpha = animator.addProperty()
        self._alpha.speed = 8.0
        self._alpha.target = 0.0
        self._alpha.current = 0.0

        self._parent_widget = None

    def set_parent_widget(self, widget):
        """
        Set the parent widget (typically GraphView).

        Args:
            widget: Parent widget that owns this overlay
        """
        self._parent_widget = widget

    @property
    def alpha(self):
        """Current alpha value (0.0 to 1.0)."""
        return self._alpha.current

    @property
    def is_visible(self):
        """Returns True if the widget is visible (alpha > 0.01)."""
        return self._alpha.current > 0.01

    @property
    def is_showing(self):
        """Returns True if the widget is showing or animating to show (based on target state)."""
        return self._alpha.target > 0.5

    def show(self):
        """Animate the widget to fully visible."""
        self._alpha.target = 1.0
        if self._parent_widget:
            self._parent_widget.update()

    def hide(self):
        """Animate the widget to fully hidden."""
        self._alpha.target = 0.0
        if self._parent_widget:
            self._parent_widget.update()

    def render(self, *args, **kwargs):
        """
        Abstract render method to be implemented by subclasses.

        Subclasses should implement their specific rendering logic here.
        """
        raise NotImplementedError("Subclasses must implement render()")
