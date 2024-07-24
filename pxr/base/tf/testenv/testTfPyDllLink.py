#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import os
import subprocess
import sys
import unittest

from typing import Dict, Iterable


TEST_ROOT = os.getcwd()
MODULE_NAME = "testTfPyDllLinkModule"
MODULE_DIR = os.path.join(TEST_ROOT, MODULE_NAME)
if not os.path.isdir(MODULE_DIR):
    raise ValueError(f"expected module directory not found - testenv not copied correctly: {MODULE_DIR}")

BAD_PATH = os.path.join(MODULE_DIR, "ImplementationBad")
GOOD_PATH = os.path.join(MODULE_DIR, "ImplementationGood")

for implementation_dir in (BAD_PATH, GOOD_PATH):
    dll_path = os.path.join(implementation_dir, "testTfPyDllLinkImplementation.dll")
    if not os.path.isfile(dll_path):
        raise ValueError(f"expected dll not found: {dll_path}")


def append_to_pathvar(env: Dict[str, str], var_name: str, new_entries: Iterable[str]):
    """Appends the given new_entries to the end of the path variable given by var_name"""
    path = env.get(var_name, "").split(os.pathsep)
    for x in new_entries:
        if x not in path:
            path.append(x)
    env[var_name] = os.pathsep.join(path)


class TestPyDllLink(unittest.TestCase):
    """Test that on windows, by modifying the PATH, we can import required
    python modules, and their .dlls, and they are searched in the correct order.
    """

    # We can't run the tests inside this process, as we want to change the
    # linkage we get when we import the same module name - so we need to shell
    # out

    def run_import_test(self, path: Iterable[str]) -> int:
        new_env = dict(os.environ)
        append_to_pathvar(new_env, "PATH", path)
        append_to_pathvar(new_env, "PYTHONPATH", [TEST_ROOT])
        cmd = f"import sys, {MODULE_NAME}; sys.exit({MODULE_NAME}.call_implementation())"
        return subprocess.call([sys.executable, "-c", cmd], env=new_env)

    def test_no_path(self):
        # with nothing adding to the path, it should fail to find testTfPyDllLinkImplementation.dll, and
        # error (with something other than 0xBAD)
        self.assertNotIn(self.run_import_test([]), [0, 0xBAD])

    def test_bad_path(self):
        # with only the bad path, it should return 0xBAD
        self.assertEqual(self.run_import_test([BAD_PATH]), 0xBAD)

    def test_good_path(self):
        # with only the good path, it should return 0
        self.assertEqual(self.run_import_test([GOOD_PATH]), 0)

    def test_bad_good_path(self):
        # with both bad and good path, but bad path first, should return 0xBAD
        self.assertEqual(self.run_import_test([BAD_PATH, GOOD_PATH]), 0xBAD)

    def test_good_bad_path(self):
        # with both good and bad path, but good path first, should return 0
        self.assertEqual(self.run_import_test([GOOD_PATH, BAD_PATH]), 0)


if __name__ == '__main__':
    unittest.main()
