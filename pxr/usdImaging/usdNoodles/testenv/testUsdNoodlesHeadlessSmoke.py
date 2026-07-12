#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Smoke tests for the headless noodles rendering pipeline.

Validates that the entire headless stack works end-to-end:
Qt offscreen -> GL init -> render -> framebuffer capture -> image.

This is the highest-risk test: if these pass, the headless pipeline
is functional and all other headless tests can build on it.
"""

import os
import tempfile
import unittest
from pathlib import Path

try:
    from tests.headless.base import NoodlesHeadlessTestCase
    from tests.headless.screenshot import image_is_nonempty

    _has_headless = True
except ImportError:
    _has_headless = False
    NoodlesHeadlessTestCase = unittest.TestCase


@unittest.skipUnless(_has_headless, "Headless harness not available")
class TestHeadlessGLInit(NoodlesHeadlessTestCase):
    """Verify GL context initializes in offscreen mode."""

    def test_gl_initializes(self):
        """GraphView should report initialized after setup."""
        self.assertTrue(
            self.harness.graph_view.initialized,
            "GraphView GL did not initialize in offscreen mode",
        )

    def test_graph_view_exists(self):
        """GraphView widget should be instantiated."""
        self.assertIsNotNone(self.harness.graph_view)

    def test_font_atlas_loaded(self):
        """Font atlas should be loaded (needed for text rendering)."""
        self.assertIsNotNone(self.harness.graph_view.fontAtlas)

    def test_shader_library_loaded(self):
        """Shader library should be compiled."""
        self.assertIsNotNone(self.harness.graph_view.shaderLibrary)


@unittest.skipUnless(_has_headless, "Headless harness not available")
class TestHeadlessScreenshot(NoodlesHeadlessTestCase):
    """Verify screenshot capture produces valid images."""

    def test_render_produces_image(self):
        """capture_screenshot should return a QImage with nonzero dimensions."""
        image = self.harness.capture_screenshot()
        self.assertGreater(image.width(), 0, "Screenshot width is 0")
        self.assertGreater(image.height(), 0, "Screenshot height is 0")

    def test_save_screenshot_to_file(self):
        """Screenshot should be saveable as PNG."""
        with tempfile.NamedTemporaryFile(suffix=".png", delete=False) as f:
            tmp_path = f.name
        try:
            result = self.harness.save_screenshot(tmp_path)
            self.assertTrue(result, "save_screenshot returned False")
            self.assertTrue(os.path.exists(tmp_path), "Screenshot file was not created")
            self.assertGreater(os.path.getsize(tmp_path), 0, "Screenshot file is empty")
        finally:
            if os.path.exists(tmp_path):
                os.unlink(tmp_path)


@unittest.skipUnless(_has_headless, "Headless harness not available")
class TestHeadlessGraphLoading(NoodlesHeadlessTestCase):
    """Verify graph loading works in headless mode."""

    def _find_asset_path(self, filename):
        """Find a test asset file relative to the usd_noodles package."""
        # Try the pxr.UsdNoodles assets path
        try:
            from pxr.UsdNoodles import graphView as gv_mod

            assets_dir = Path(gv_mod.__file__).parent / "assets"
            candidate = assets_dir / filename
            if candidate.exists():
                return str(candidate)
        except (ImportError, AttributeError):
            pass

        # Fallback: search relative to test directory
        test_dir = Path(__file__).parent.parent
        for subdir in ["src/usd_noodles/assets", "assets"]:
            candidate = test_dir / subdir / filename
            if candidate.exists():
                return str(candidate)

        return None

    def test_load_json_graph(self):
        """Loading a JSON graph should populate nodes."""
        asset_path = self._find_asset_path("test_single_node.json")
        if asset_path is None:
            self.skipTest("test_single_node.json asset not found")

        self.harness.load_json_graph(asset_path)
        self.assertGreater(
            len(self.harness.graph_view.nodes),
            0,
            "No nodes loaded from JSON graph",
        )

    def test_load_empty_blueprint(self):
        """Loading an empty Blueprint should result in an empty graph."""
        # setUp already loads an empty Blueprint
        self.assertEqual(self.helper.get_node_count(), 0)

    def test_image_not_all_black_with_graph(self):
        """Screenshot of a loaded graph should contain non-black pixels."""
        asset_path = self._find_asset_path("body_graph.json")
        if asset_path is None:
            self.skipTest("body_graph.json asset not found")

        self.harness.load_json_graph(asset_path)
        image = self.harness.capture_screenshot()
        self.assertTrue(
            image_is_nonempty(image),
            "Screenshot is all-black — GL may not be rendering",
        )

    def test_load_usda_fixture(self):
        """Loading the 3-node USDA fixture should produce 3 nodes."""
        from pxr import Usd

        fixture_path = Path(__file__).parent / "fixtures" / "headless_test_3nodes.usda"
        if not fixture_path.exists():
            self.skipTest("headless_test_3nodes.usda fixture not found")

        stage = Usd.Stage.Open(str(fixture_path))
        self.harness.load_blueprint(stage, "/TestBP")
        self.assertEqual(
            self.helper.get_node_count(),
            3,
            "Expected 3 nodes from USDA fixture",
        )
