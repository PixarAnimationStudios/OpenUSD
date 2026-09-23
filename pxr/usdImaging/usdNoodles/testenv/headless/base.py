#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

"""
Base test class for headless noodles tests.

Provides shared QApplication/GL setup (class-level) and per-test
fresh graph state. All headless tests should inherit from
NoodlesHeadlessTestCase.
"""

import unittest

from .harness import GLNotAvailableError, NoodlesTestHarness
from .helpers import GraphTestHelper


class NoodlesHeadlessTestCase(unittest.TestCase):
    """Base class for headless noodles tests with GL rendering.

    setUpClass creates a shared QApplication and GraphView (expensive).
    setUp provides each test method with a fresh empty graph and
    cleared undo stack.

    If the offscreen platform cannot provide an OpenGL context,
    all tests in the class are skipped automatically.
    """

    harness = None
    helper = None
    _stage = None
    _bp_path = None

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.harness = NoodlesTestHarness()
        try:
            cls.harness.setup()
        except GLNotAvailableError as e:
            cls.harness = None
            raise unittest.SkipTest(f"Skipping headless tests — GL not available: {e}")
        except (ImportError, OSError) as e:
            cls.harness = None
            raise unittest.SkipTest(
                f"Skipping headless tests — Qt/PySide6 not available: {e}"
            )
        cls.helper = GraphTestHelper(cls.harness)

    @classmethod
    def tearDownClass(cls):
        if cls.harness is not None:
            cls.harness.teardown()
            cls.harness = None
            cls.helper = None
        super().tearDownClass()

    def setUp(self):
        super().setUp()
        # Clear undo stack
        self.helper.clear_undo_stack()
        # Create fresh empty stage with Blueprint
        self._stage, self._bp_path = self.harness.create_empty_stage()
        # Load into GraphView
        self.harness.load_blueprint(self._stage, self._bp_path)

    def tearDown(self):
        self._stage = None
        self._bp_path = None
        super().tearDown()

    @property
    def stage(self):
        return self._stage

    @property
    def bp_path(self):
        return self._bp_path
