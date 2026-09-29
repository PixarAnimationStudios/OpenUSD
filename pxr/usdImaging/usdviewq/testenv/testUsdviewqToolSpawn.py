#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Tests that usdview spawns helper tools without letting a path decide what
# gets run. Paths reaching these code paths come from the content of an opened
# layer, so a path such as 'layer.usda;rm -rf /' must be passed through as a
# single argument rather than parsed as a command.

import os
import subprocess
import sys
import unittest

from pxr.UsdUtils import toolPaths

# A path holding characters that a shell would treat as command syntax, and
# which a package-relative path legitimately resembles in its use of brackets.
EVIL_PATH = "layer.usda;touch pwned.txt"
PACKAGE_RELATIVE_PATH = "/tmp/bundle.usdz[sub.usda]"


class SpawnRecorder(object):
    """Records the argument lists that would have been spawned, standing in for
    both subprocess.Popen and subprocess.call.
    """

    def __init__(self):
        self.calls = []

    def __call__(self, args, **kwargs):
        self.calls.append(args)
        return self

    # Popen protocol, enough for callers that hold the returned object.
    def wait(self):
        return 0

    def poll(self):
        return 0


class ToolSpawnTestCase(unittest.TestCase):
    """Fixture that intercepts process creation and forbids shell use."""

    def setUp(self):
        self.recorder = SpawnRecorder()

        self._saved = (subprocess.Popen, subprocess.call, os.system)
        subprocess.Popen = self.recorder
        subprocess.call = self.recorder

        def forbidden(command):
            self.fail("os.system was called with %r. Helper tools must be "
                      "spawned without a shell." % (command,))

        os.system = forbidden

        # Resolve to something that exists so that resolution never fails, and
        # is not a .cmd, so the metacharacter guard stays out of the way.
        self._savedFind = toolPaths.FindUsdBinary
        toolPaths.FindUsdBinary = lambda name: os.path.join(
            os.path.sep + "usr", "bin", name)

    def tearDown(self):
        subprocess.Popen, subprocess.call, os.system = self._saved
        toolPaths.FindUsdBinary = self._savedFind

    def assertSingleSpawn(self):
        self.assertEqual(len(self.recorder.calls), 1)
        args = self.recorder.calls[0]
        # A string would mean the command is subject to shell parsing.
        self.assertIsInstance(args, list)
        for arg in args:
            self.assertIsInstance(arg, str)
        return args


class TestRunUsdBinary(ToolSpawnTestCase):

    def test_pathIsPassedAsSingleArgument(self):
        """A path containing shell syntax survives as one unsplit argument."""

        toolPaths.RunUsdBinary("usdview", paths=[EVIL_PATH])

        args = self.assertSingleSpawn()
        self.assertEqual(args[-1], EVIL_PATH)
        self.assertEqual(args.count(EVIL_PATH), 1)

    def test_pathsFollowEndOfOptions(self):
        """Paths are separated from options so one starting with '-' is not
        mistaken for an option."""

        toolPaths.RunUsdBinary("usdview", paths=["--traceToFile"])

        args = self.assertSingleSpawn()
        self.assertIn("--", args)
        self.assertEqual(args[args.index("--") + 1], "--traceToFile")

    def test_optionsPrecedeEndOfOptions(self):
        """Caller-supplied options stay ahead of the separator, where they are
        still recognized as options."""

        toolPaths.RunUsdBinary(
            "usdedit", options=["-n", "-p", "x.tmp"], paths=[EVIL_PATH])

        args = self.assertSingleSpawn()
        separator = args.index("--")
        self.assertLess(args.index("-n"), separator)
        self.assertLess(args.index("-p"), separator)
        self.assertGreater(args.index(EVIL_PATH), separator)

    def test_packageRelativePathIsPreserved(self):
        """A layer inside a package has a path that is not a file on disk, and
        must still be passed through unchanged."""

        toolPaths.RunUsdBinary("usdview", paths=[PACKAGE_RELATIVE_PATH])

        args = self.assertSingleSpawn()
        self.assertEqual(args[-1], PACKAGE_RELATIVE_PATH)

    def test_noSeparatorWithoutPaths(self):
        """A tool invoked with no paths needs no separator."""

        toolPaths.RunUsdBinary("usdGenSchema", options=["--validate"])

        args = self.assertSingleSpawn()
        self.assertNotIn("--", args)

    def test_missingToolRaises(self):
        toolPaths.FindUsdBinary = lambda name: None

        with self.assertRaises(RuntimeError):
            toolPaths.RunUsdBinary("usdview", paths=["a.usda"])

    def test_cmdWrapperRefusesMetacharacters(self):
        """When only a Windows .cmd wrapper can be found, arguments reach
        cmd.exe, which would interpret these characters. They are refused
        rather than escaped, because cmd.exe quoting cannot be done reliably.
        """
        toolPaths.FindUsdBinary = lambda name: "C:\\usd\\bin\\%s.cmd" % name

        for path in ("a&b.usda", "a|b.usda", "a>b.usda", "a%PATH%b.usda",
                     "a^b.usda", "a(b).usda", 'a"b.usda'):
            with self.assertRaises(RuntimeError):
                toolPaths.RunUsdBinary("usdview", paths=[path])

        self.assertEqual(len(self.recorder.calls), 0)

    def test_cmdWrapperAllowsOrdinaryPath(self):
        """The wrapper fallback still runs for paths it cannot misinterpret."""

        toolPaths.FindUsdBinary = lambda name: "C:\\usd\\bin\\%s.cmd" % name

        toolPaths.RunUsdBinary("usdview", paths=["C:\\assets\\a.usda"])

        args = self.assertSingleSpawn()
        self.assertEqual(args[-1], "C:\\assets\\a.usda")


class TestSpawnUsdTool(ToolSpawnTestCase):
    """Tests the usdview-side wrapper, which adds reporting and rejects paths
    that do not identify anything.
    """

    def test_spawnsWithSeparatedPath(self):
        from pxr.Usdviewq.common import SpawnUsdTool

        self.assertTrue(SpawnUsdTool("usdview", [EVIL_PATH]))

        args = self.assertSingleSpawn()
        self.assertIn("--", args)
        self.assertEqual(args[-1], EVIL_PATH)

    def test_unresolvedPathsAreRefused(self):
        """Sdf reports an empty realPath for a layer with no backing file, and
        usdview substitutes 'unknown' when a muted layer cannot be resolved.
        Neither can be opened, so no process should start.
        """
        from pxr.Usdviewq.common import SpawnUsdTool

        for path in ("", "unknown"):
            self.assertFalse(SpawnUsdTool("usdview", [path]))

        self.assertEqual(len(self.recorder.calls), 0)

    def test_failureIsReportedNotRaised(self):
        """A missing tool is reported to the user rather than raised into the
        Qt event loop, where it would be unhandled.
        """
        from pxr.Usdviewq.common import SpawnUsdTool

        toolPaths.FindUsdBinary = lambda name: None

        self.assertFalse(SpawnUsdTool("usdview", ["a.usda"]))


class TestContextMenuItems(ToolSpawnTestCase):
    """Tests the two menu items named in GHSA-qgc5-vc49-x7jw, driving them the
    way the interface does.
    """

    def test_usdviewLayerMenuItem(self):
        from pxr.Usdviewq.layerStackContextMenu import UsdviewLayerMenuItem

        class FakeItem:
            layerPath = EVIL_PATH

        UsdviewLayerMenuItem(FakeItem()).RunCommand()

        args = self.assertSingleSpawn()
        self.assertEqual(args[-1], EVIL_PATH)
        self.assertIn("--", args)

    def test_openLayerMenuItem(self):
        """This item spawns usdedit, which historically passed the path to a
        shell of its own.
        """
        from pxr.Usdviewq.layerStackContextMenu import OpenLayerMenuItem

        class FakeItem:
            layerPath = EVIL_PATH

        OpenLayerMenuItem(FakeItem()).RunCommand()

        args = self.assertSingleSpawn()
        self.assertEqual(args[-1], EVIL_PATH)
        self.assertIn("--", args)

    def test_isolateAssetMenuItem(self):
        from pxr.Usdviewq.primContextMenuItems import IsolateAssetMenuItem

        # Built without a selection so that __init__ does no stage work; the
        # spawn path is what matters here.
        item = IsolateAssetMenuItem.__new__(IsolateAssetMenuItem)
        item._assetName = "ProofOfConcept"
        item._filePath = EVIL_PATH

        item.RunCommand()

        args = self.assertSingleSpawn()
        self.assertEqual(args[-1], EVIL_PATH)
        self.assertIn("--", args)


class TestNoShellUse(unittest.TestCase):
    """Guards against a shell being reintroduced in the modules that spawn
    helper tools."""

    def test_sourcesDoNotCallOsSystem(self):
        from pxr.Usdviewq import (common, configController,
                                  layerStackContextMenu, primContextMenuItems)

        import inspect
        for module in (common, configController, layerStackContextMenu,
                       primContextMenuItems):
            source = inspect.getsource(module)
            self.assertNotIn("os.system", source,
                "%s calls os.system. Spawn helper tools with "
                "toolPaths.RunUsdBinary instead." % module.__name__)
            self.assertNotIn("shell=True", source,
                "%s spawns a shell. Pass an argument list instead."
                % module.__name__)


if __name__ == "__main__":
    unittest.main(verbosity=2)
