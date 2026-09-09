#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
NoodlesTestHarness — headless Qt/OpenGL setup for testing GraphView.

Handles QApplication creation, offscreen platform configuration,
GraphView instantiation, and screenshot capture. Works on Linux
(OSMesa), macOS (Apple software GL), and Windows (desktop OpenGL).

Usage:
    harness = NoodlesTestHarness()
    harness.setup()
    harness.load_json_graph("path/to/graph.json")
    image = harness.capture_screenshot()
    harness.teardown()
"""

import os
import sys


class GLNotAvailableError(RuntimeError):
    """Raised when the offscreen platform cannot provide an OpenGL context."""

    pass


def _configure_offscreen_env():
    """Set environment variables for headless Qt/GL before any Qt import.

    Platform strategy:
    - Linux: Use QT_QPA_PLATFORM=offscreen + OSMesa (no display in CI).
    - macOS: Use native Cocoa plugin — the macOS offscreen plugin does NOT
      support OpenGL. Use WA_DontShowOnScreen to hide the window instead.
    - Windows: Use QT_QPA_PLATFORM=offscreen + force desktop OpenGL
      (ANGLE doesn't support GLSL 4.1 Core shaders).

    MUST be called before QApplication construction.
    """
    if sys.platform == "linux":
        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
    elif sys.platform == "win32":
        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
        os.environ.setdefault("QT_OPENGL", "desktop")
    # macOS: do NOT set QT_QPA_PLATFORM — use native Cocoa plugin


class NoodlesTestHarness:
    """Manages headless GraphView lifecycle for testing.

    All Qt/pxr imports are deferred to setup() to avoid triggering
    PySide6 loading during test listing (which fails if shared libs
    like libpcre.so.1 are missing from the PAR).
    """

    def __init__(self, width=800, height=600):
        self.width = width
        self.height = height
        self.app = None
        self.graph_view = None
        self._owns_app = False

    def setup(self):
        """Create QApplication and GraphView in offscreen mode.

        Raises:
            GLNotAvailableError: If the offscreen platform cannot provide
                an OpenGL context (e.g. macOS without OSMesa).
        """
        # Configure env BEFORE importing Qt
        _configure_offscreen_env()

        # Now import Qt (deferred from module level to avoid PySide6
        # import failures during test listing)
        from pxr.Usdviewq.qt import QtCore, QtWidgets

        self._QtCore = QtCore

        # Force desktop OpenGL on Windows before QApplication construction
        if sys.platform == "win32":
            QtCore.QCoreApplication.setAttribute(
                QtCore.Qt.ApplicationAttribute.AA_UseDesktopOpenGL
            )

        # Create QApplication if one doesn't exist (singleton constraint)
        self.app = QtWidgets.QApplication.instance()
        if self.app is None:
            self.app = QtWidgets.QApplication([])
            self._owns_app = True

        # Import GraphView after QApplication exists
        from pxr.UsdNoodles.graphView import GraphView

        try:
            self.graph_view = GraphView(parent=None)
        except Exception as e:
            raise GLNotAvailableError(
                f"Failed to create GraphView — GL may not be available "
                f"on this offscreen platform: {e}"
            ) from e

        self.graph_view.resize(self.width, self.height)

        # WA_DontShowOnScreen + show() makes isVisible() return True
        # without needing a display server. This bypasses the guard
        # in paintGL() at graphView.py:2580.
        self.graph_view.setAttribute(
            QtCore.Qt.WidgetAttribute.WA_DontShowOnScreen, True
        )
        self.graph_view.show()

        # Force GL initialization (loads fonts, shaders, render managers)
        self.graph_view.initializeGL()

        # Verify GL actually initialized
        if not self.graph_view.initialized:
            raise GLNotAvailableError(
                "GraphView GL initialization failed — the offscreen "
                "platform may not support OpenGL. On Linux, ensure "
                "OSMesa is available."
            )

    def teardown(self):
        """Clean up GL resources and widgets."""
        if self.graph_view is not None:
            self.graph_view.close()
            self.graph_view.deleteLater()
            self.graph_view = None

        if self._owns_app and self.app is not None:
            self.app.processEvents()

    def create_empty_stage(self):
        """Create an in-memory USD stage with an empty Blueprint prim.

        Returns:
            tuple: (stage, blueprint_path_str) e.g. (stage, "/TestBP")
        """
        from pxr import Usd

        stage = Usd.Stage.CreateInMemory()
        bp_path = "/TestBP"
        stage.DefinePrim(bp_path, "Blueprint")
        return stage, bp_path

    def load_blueprint(self, stage, prim_path):
        """Load a Blueprint graph into the GraphView."""
        self.graph_view.loadBlueprint(stage, prim_path)

    def load_json_graph(self, json_path):
        """Load a JSON graph fixture into the GraphView."""
        self.graph_view._loadSampleGraph(json_path)
        if self.graph_view.nodes:
            self.graph_view.frameScene()

    def render_frame(self):
        """Force a single render pass (offscreen)."""
        gv = self.graph_view
        gv.makeCurrent()
        gv._paintGLInner()
        gv.doneCurrent()

    def capture_screenshot(self):
        """Render and capture the framebuffer as a QImage.

        Returns:
            QImage: The captured framebuffer contents
        """
        self.render_frame()
        self.graph_view.makeCurrent()
        image = self.graph_view.grabFrameBuffer()
        self.graph_view.doneCurrent()
        return image

    def save_screenshot(self, path):
        """Render and save a screenshot to disk as PNG.

        Returns:
            bool: True if saved successfully
        """
        image = self.capture_screenshot()
        return image.save(path, "PNG")
