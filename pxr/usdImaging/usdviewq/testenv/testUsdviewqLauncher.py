#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import unittest
import sys

from pxr.Usdviewq import Launcher, InvalidUsdviewOption


class ValidationOnlyLauncher(Launcher):
    """A Usdview launcher which only performs option validation when its Run()
    method is called. The main application is not started.
    """

    def __init__(self):
        super(ValidationOnlyLauncher, self).__init__()
        self._args = None

    def Run(self, args=None):
        """Run the launcher, and optionally provide a list of command line
        arguments. By default, no arguments are used.
        """
        args = args or []
        self._args = [sys.argv[0]] + args
        super(ValidationOnlyLauncher, self).Run()

    def ParseOptions(self, parser):
        """Parse the given command line arguments rather than the test's command
        line arguments.
        """

        return parser.parse_args(self._args)

    def _Launcher__LaunchProcess(self, arg_parse_result):
        """Do nothing. We don't want to launch the main application."""

        return


class TestLauncher(unittest.TestCase):

    def test_noOptions(self):
        """Test the launcher with no arguments."""

        ValidationOnlyLauncher().Run()

    def test_cameraFlag(self):
        """Only a valid non-root prim path or a camera name should be
        acceptable.
        """

        # Non-root prim path.
        ValidationOnlyLauncher().Run(["--camera", "/camera"])

        # Relative prim path is converted to absolute (and prints a warning).
        ValidationOnlyLauncher().Run(["--camera", "cam/era"])

        # Camera name.
        ValidationOnlyLauncher().Run(["--camera", "camera"])

        with self.assertRaises(InvalidUsdviewOption):
            ValidationOnlyLauncher().Run(["--camera", "/"])

        with self.assertRaises(InvalidUsdviewOption):
            ValidationOnlyLauncher().Run(["--camera", "/camera.prop"])

    def test_clearAndDefaultSettingsFlags(self):
        """--clearsettings and --defaultsettings can be used separately, but not
        together.
        """

        ValidationOnlyLauncher().Run(["--clearsettings"])
        ValidationOnlyLauncher().Run(["--defaultsettings"])

        with self.assertRaises(InvalidUsdviewOption):
            ValidationOnlyLauncher().Run(
                ["--clearsettings", "--defaultsettings"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
