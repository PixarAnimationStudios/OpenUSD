#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

# Maximum Z-depth for orthographic projection (rendering depth range is -MAX_RENDER_DEPTH to +MAX_RENDER_DEPTH)
MAX_RENDER_DEPTH = 100

# Depth step for z-ordering.  Node backgrounds use ``zOrder * DEPTH_STEP``,
# text and ports use ``zOrder * DEPTH_STEP + DEPTH_STEP * 0.5`` (between
# this node's background and the next).  Supports ~1000 unique z-order
# levels before depth precision issues arise.
DEPTH_STEP = 0.001


def node_content_depth(z_order):
    """Depth of a node's content (text, ports, row/title icons): half a
    depth-step in front of the node background, which sits at
    ``z_order * DEPTH_STEP``.

    Text, ports, and icons must all share this band so they raise together when
    a node's ``zOrder`` changes on selection. Mirrors
    ``RenderConfig::contentDepth`` on the C++ side.
    """
    return z_order * DEPTH_STEP + DEPTH_STEP * 0.5


# Pin row texture decorations. The icons are square quads sized from the
# visible row height so they scale with the renderer's font settings.
ROW_MINUS_ICON_NAME = "row-minus.png"
RELATIONSHIP_ROW_ICON_NAME = "relationship-arrows.png"
ROW_ICON_SIZE_RATIO = 0.64
RELATIONSHIP_ROW_ICON_SIZE_RATIO = 0.72
ROW_ICON_GAP_RATIO = 0.25
