#!/pxrpythonsubst
#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#


"""
Unit tests for the C++ TextLayout module.

Tests the following functionality:
- GlyphMetrics struct and conversion from Python glyph objects
- TextLayout.calculateTextWidth() with C++ acceleration
- TextLayout.generateTextVertices() for GPU vertex generation
- TextLayout.calculateTextWidthBatch() for batch operations
- TextLayout.calculatePortPositions() for node port positioning

To run these tests:
    python testUsdNoodlesTextLayout.py

Expected results:
    - All tests pass
    - Performance benchmarks show 2-3x speedup over pure Python
"""

import time
import unittest

from pxr import Gf


# Mock glyph class matching the structure from models.py
class MockGlyph:
    """Mock glyph for testing - matches the structure used by FontAtlas."""

    def __init__(
        self, unicode_val, advance, plane_min, plane_max, atlas_min, atlas_max
    ):
        self.unicode = unicode_val
        self.advance = advance
        self.planeBounds = Gf.Range2d(Gf.Vec2d(*plane_min), Gf.Vec2d(*plane_max))
        self.atlasBounds = Gf.Range2d(Gf.Vec2d(*atlas_min), Gf.Vec2d(*atlas_max))


def create_test_glyph_map():
    """Create a simple test glyph map for ASCII characters."""
    glyph_map = {}

    # Create basic ASCII glyphs (A-Z, a-z, 0-9, space)
    for i, char in enumerate(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 "
    ):
        unicode_val = ord(char)
        # Simplified metrics for testing
        advance = 0.5 if char == " " else 0.6
        glyph_map[unicode_val] = MockGlyph(
            unicode_val=unicode_val,
            advance=advance,
            plane_min=(0.0, -0.2),
            plane_max=(0.5, 0.8),
            atlas_min=(i * 0.01, 0.0),
            atlas_max=(i * 0.01 + 0.01, 1.0),
        )

    return glyph_map


class TestTextLayoutCppBinding(unittest.TestCase):
    """Test that the C++ TextLayout module binds correctly."""

    @classmethod
    def setUpClass(cls):
        """Try to import the C++ module."""
        try:
            from pxr.UsdNoodles._usdNoodles import Glyph, TextLayout, TextVertexResult

            cls.TextLayout = TextLayout
            cls.Glyph = Glyph
            cls.TextVertexResult = TextVertexResult
            cls.has_cpp = True
        except ImportError as e:
            cls.has_cpp = False
            cls.import_error = str(e)

    def test_cpp_module_available(self):
        """Test that C++ module can be imported."""
        if not self.has_cpp:
            self.skipTest(f"C++ module not available: {self.import_error}")
        self.assertTrue(self.has_cpp, "C++ TextLayout module should be importable")

    def test_glyph_metrics_struct(self):
        """Test Glyph struct creation and access."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        glyph = self.Glyph()
        self.assertEqual(glyph.unicode, 0)
        self.assertEqual(glyph.advance, 0.0)


class TestCalculateTextWidth(unittest.TestCase):
    """Test TextLayout.calculateTextWidth()."""

    @classmethod
    def setUpClass(cls):
        try:
            from pxr.UsdNoodles._usdNoodles import TextLayout

            cls.TextLayout = TextLayout
            cls.has_cpp = True
        except ImportError:
            cls.has_cpp = False
        cls.glyph_map = create_test_glyph_map()

    def test_empty_string(self):
        """Test width of empty string."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        width = self.TextLayout.calculateTextWidth("", 24.0, self.glyph_map)
        self.assertEqual(width, 0.0)

    def test_single_character(self):
        """Test width of single character."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        # 'A' has advance 0.6, fontSize 24 -> width = 0.6 * 24 = 14.4
        width = self.TextLayout.calculateTextWidth("A", 24.0, self.glyph_map)
        self.assertAlmostEqual(width, 14.4, places=5)

    def test_multiple_characters(self):
        """Test width of multiple characters."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        # "ABC" = 0.6 + 0.6 + 0.6 = 1.8 * 24 = 43.2
        width = self.TextLayout.calculateTextWidth("ABC", 24.0, self.glyph_map)
        self.assertAlmostEqual(width, 43.2, places=5)

    def test_with_space(self):
        """Test width including space character."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        # "A B" = 0.6 + 0.5 + 0.6 = 1.7 * 24 = 40.8
        width = self.TextLayout.calculateTextWidth("A B", 24.0, self.glyph_map)
        self.assertAlmostEqual(width, 40.8, places=5)

    def test_different_font_sizes(self):
        """Test width scales correctly with font size."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        width_12 = self.TextLayout.calculateTextWidth("Test", 12.0, self.glyph_map)
        width_24 = self.TextLayout.calculateTextWidth("Test", 24.0, self.glyph_map)

        self.assertAlmostEqual(width_24, width_12 * 2, places=5)


class TestGenerateTextVertices(unittest.TestCase):
    """Test TextLayout.generateTextVertices()."""

    @classmethod
    def setUpClass(cls):
        try:
            from pxr.UsdNoodles._usdNoodles import TextLayout

            cls.TextLayout = TextLayout
            cls.has_cpp = True
        except ImportError:
            cls.has_cpp = False
        cls.glyph_map = create_test_glyph_map()

    def test_empty_string(self):
        """Test vertex generation for empty string."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        result = self.TextLayout.generateTextVertices(
            "", 0.0, 0.0, 0.0, 24.0, self.glyph_map, 0.8, 0.0
        )
        vertex_data, cursor_x, char_count = result

        self.assertEqual(len(vertex_data), 0)
        self.assertEqual(cursor_x, 0.0)
        self.assertEqual(char_count, 0)

    def test_single_character_vertices(self):
        """Test vertex generation for single character."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        result = self.TextLayout.generateTextVertices(
            "A", 0.0, 0.0, 1.0, 24.0, self.glyph_map, 0.8, 0.0
        )
        vertex_data, cursor_x, char_count = result

        # 1 char = 6 vertices * 6 floats = 36 floats
        self.assertEqual(len(vertex_data), 36)
        self.assertEqual(char_count, 1)

    def test_multiple_characters_vertices(self):
        """Test vertex generation for multiple characters."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        result = self.TextLayout.generateTextVertices(
            "Test", 0.0, 0.0, 1.0, 24.0, self.glyph_map, 0.8, 0.0
        )
        vertex_data, cursor_x, char_count = result

        # 4 chars = 4 * 6 vertices * 6 floats = 144 floats
        self.assertEqual(len(vertex_data), 144)
        self.assertEqual(char_count, 4)

    def test_vertex_format(self):
        """Test vertex data format (x, y, z, s, t, nodeIndex)."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        result = self.TextLayout.generateTextVertices(
            "A", 10.0, 20.0, 5.0, 24.0, self.glyph_map, 0.8, 3.0
        )
        vertex_data, cursor_x, char_count = result

        # Check first vertex
        x, y, z, s, t, node_idx = vertex_data[0:6]

        # z should be depth value
        self.assertEqual(z, 5.0)
        # nodeIndex should be 3.0
        self.assertEqual(node_idx, 3.0)


class TestCalculateTextWidthBatch(unittest.TestCase):
    """Test TextLayout.calculateTextWidthBatch()."""

    @classmethod
    def setUpClass(cls):
        try:
            from pxr.UsdNoodles._usdNoodles import TextLayout

            cls.TextLayout = TextLayout
            cls.has_cpp = True
        except ImportError:
            cls.has_cpp = False
        cls.glyph_map = create_test_glyph_map()

    def test_batch_calculation(self):
        """Test batch width calculation matches individual calls."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        texts = ["Hello", "World", "Test", "Node"]
        font_size = 18.0

        # Get batch results
        batch_widths = self.TextLayout.calculateTextWidthBatch(
            texts, font_size, self.glyph_map
        )

        # Compare with individual calls
        for i, text in enumerate(texts):
            individual_width = self.TextLayout.calculateTextWidth(
                text, font_size, self.glyph_map
            )
            self.assertAlmostEqual(batch_widths[i], individual_width, places=5)


class TestCalculatePortPositions(unittest.TestCase):
    """Test TextLayout.calculatePortPositions()."""

    @classmethod
    def setUpClass(cls):
        try:
            from pxr.UsdNoodles._usdNoodles import TextLayout

            cls.TextLayout = TextLayout
            cls.has_cpp = True
        except ImportError:
            cls.has_cpp = False
        cls.glyph_map = create_test_glyph_map()

    def test_port_position_count(self):
        """Test that correct number of positions are returned."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        input_pins = ["A", "B", "C"]
        output_pins = ["X", "Y"]
        node_pos = Gf.Vec2d(0.0, 0.0)
        node_size = Gf.Vec2d(200.0, 150.0)

        positions = self.TextLayout.calculatePortPositions(
            input_pins,
            output_pins,
            18.0,  # fontSize
            5.0,  # portSpacing
            node_pos,
            node_size,
            self.glyph_map,
            16.0,  # marginH
            40.0,  # titleHeight
        )

        # Should return input_count + output_count positions
        self.assertEqual(len(positions), 5)

    def test_port_positions_respect_row_slots(self):
        """Output rows should use authored row slots instead of stacking below inputs."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        input_pins = ["inputs", "foo", "middle", "bar"]
        output_pins = ["outputs", "left", "right"]
        node_pos = Gf.Vec2d(0.0, 0.0)
        node_size = Gf.Vec2d(200.0, 150.0)

        positions = self.TextLayout.calculatePortPositions(
            input_pins,
            output_pins,
            18.0,
            5.0,
            node_pos,
            node_size,
            self.glyph_map,
            16.0,
            40.0,
            [0, 1, 4, 5],
            [2, 3, 6],
        )

        self.assertEqual(len(positions), 7)
        self.assertLess(positions[1][1], positions[4][1])
        self.assertLess(positions[4][1], positions[5][1])
        self.assertLess(positions[5][1], positions[2][1])
        self.assertLess(positions[2][1], positions[3][1])
        self.assertLess(positions[3][1], positions[6][1])


class TestPerformanceBenchmark(unittest.TestCase):
    """Performance benchmarks for C++ TextLayout vs Python."""

    @classmethod
    def setUpClass(cls):
        try:
            from pxr.UsdNoodles._usdNoodles import TextLayout

            cls.TextLayout = TextLayout
            cls.has_cpp = True
        except ImportError:
            cls.has_cpp = False
        cls.glyph_map = create_test_glyph_map()

    def _python_calculate_text_width(self, text, font_size, glyph_map):
        """Pure Python implementation for comparison."""
        width = 0.0
        for ch in text:
            unicode_val = ord(ch)
            if unicode_val in glyph_map:
                width += glyph_map[unicode_val].advance
        return width * font_size

    def test_benchmark_calculate_text_width(self):
        """Benchmark calculateTextWidth: C++ vs Python."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        test_strings = [f"TestString{i}" for i in range(100)]
        iterations = 1000

        # Benchmark Python
        python_start = time.perf_counter()
        for _ in range(iterations):
            for s in test_strings:
                self._python_calculate_text_width(s, 24.0, self.glyph_map)
        python_time = time.perf_counter() - python_start

        # Benchmark C++
        cpp_start = time.perf_counter()
        for _ in range(iterations):
            for s in test_strings:
                self.TextLayout.calculateTextWidth(s, 24.0, self.glyph_map)
        cpp_time = time.perf_counter() - cpp_start

        print(f"\n  calculateTextWidth benchmark ({iterations * 100} calls):")
        print(f"    Python: {python_time * 1000:.2f}ms")
        print(f"    C++:    {cpp_time * 1000:.2f}ms")
        if cpp_time > 0:
            print(f"    Speedup: {python_time / cpp_time:.2f}x")

        # Note: Due to Python/C++ boundary crossing overhead with dict conversion,
        # the C++ version may not always be faster for single calls.
        # The main benefit is in batch operations.

    def test_benchmark_batch_vs_individual(self):
        """Benchmark batch vs individual width calculation."""
        if not self.has_cpp:
            self.skipTest("C++ module not available")

        texts = [f"NodeName{i}" for i in range(200)]
        iterations = 500

        # Benchmark individual calls
        individual_start = time.perf_counter()
        for _ in range(iterations):
            for text in texts:
                self.TextLayout.calculateTextWidth(text, 24.0, self.glyph_map)
        individual_time = time.perf_counter() - individual_start

        # Benchmark batch call
        batch_start = time.perf_counter()
        for _ in range(iterations):
            self.TextLayout.calculateTextWidthBatch(texts, 24.0, self.glyph_map)
        batch_time = time.perf_counter() - batch_start

        print(
            f"\n  Batch vs Individual benchmark ({iterations} iterations, {len(texts)} strings each):"
        )
        print(f"    Individual: {individual_time * 1000:.2f}ms")
        print(f"    Batch:      {batch_time * 1000:.2f}ms")
        if batch_time > 0:
            print(f"    Speedup: {individual_time / batch_time:.2f}x")


if __name__ == "__main__":
    unittest.main(verbosity=2)
