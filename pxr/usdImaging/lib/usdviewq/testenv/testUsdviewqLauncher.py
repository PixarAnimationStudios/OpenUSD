#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
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

    def Run(self, args=[]):
        """Run the launcher, and optionally provide a list of command line
        arguments. By default, no arguments are used.
        """

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
