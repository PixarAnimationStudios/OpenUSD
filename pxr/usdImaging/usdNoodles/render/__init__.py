#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from pxr.UsdNoodles._usdNoodles import (  # noqa: F401
    FontAtlas,
    IconRenderManager,
    LinkRenderManager,
    NodeRenderManager,
    NodeTransformFrame,
    ShaderLibrary,
    StickerRenderer,
    TextRenderManager,
)

try:
    from pxr.UsdNoodles._usdNoodles import (  # noqa: F401
        DefaultNodeRenderer,
        DummyContext,
        Glyph,
        GraffiNodeRenderer,
        GraphNodeRenderer,
        NodeVertex,
        NodeVertexCache,
        PortInfo,
        RenderProfiler,
        VertexGenerator,
    )
except ImportError:
    import logging

    logging.getLogger(__name__).debug(
        "Optional render classes not available (Qt/GL bindings not loaded)"
    )
    DefaultNodeRenderer = None
    DummyContext = None
    Glyph = None
    GraffiNodeRenderer = None
    GraphNodeRenderer = None
    NodeVertex = None
    NodeVertexCache = None
    PortInfo = None
    RenderProfiler = None
    VertexGenerator = None

NODE_VERTEX_ATTRIB_LAYOUT = [
    (0, 3, "GL_FLOAT", False, 48, 0),
    (1, 2, "GL_FLOAT", False, 48, 12),
    (2, 2, "GL_FLOAT", False, 48, 20),
    (3, 4, "GL_UNSIGNED_BYTE", True, 48, 28),
    (4, 1, "GL_FLOAT", False, 48, 32),
    (5, 4, "GL_UNSIGNED_BYTE", True, 48, 36),
    (6, 1, "GL_FLOAT", False, 48, 40),
    (7, 1, "GL_FLOAT", False, 48, 44),
]
