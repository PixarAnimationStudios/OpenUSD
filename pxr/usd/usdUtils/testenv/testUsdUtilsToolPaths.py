#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import shlex
import sys
import unittest

from pxr.UsdUtils import toolPaths


class TestUsdUtilsToolPaths(unittest.TestCase):
    def _Split(self, commandLine, windows):
        return toolPaths._SplitCommandLine(commandLine, windows)

    def testSplitCommandLineCommon(self):
        # Cases that split the same way on every platform.
        for windows in (False, True):
            split = lambda s: self._Split(s, windows)
            self.assertEqual(split(''), [])
            self.assertEqual(split('   '), [])
            self.assertEqual(split('vim'), ['vim'])
            self.assertEqual(split('emacs  -nw'), ['emacs', '-nw'])
            self.assertEqual(split('diff --label="a b" -u'),
                             ['diff', '--label=a b', '-u'])
            self.assertEqual(split('sed -i "" s/a/b/'),
                             ['sed', '-i', '', 's/a/b/'])
            # '#' does not begin a comment.
            self.assertEqual(split('x #y'), ['x', '#y'])
            with self.assertRaises(ValueError):
                split('diff "unclosed')

    def testSplitCommandLinePosix(self):
        split = lambda s: self._Split(s, windows=False)
        self.assertEqual(split("sed -i '' 's/a b/c/'"),
                         ['sed', '-i', '', 's/a b/c/'])
        self.assertEqual(split(r'/opt/my\ tools/diff -u'),
                         ['/opt/my tools/diff', '-u'])
        # Matches shlex.split exactly.
        for s in ["a 'b c' \"d e\" f\\ g", r'x\"y', "''"]:
            self.assertEqual(split(s), shlex.split(s))

    def testSplitCommandLineWindows(self):
        split = lambda s: self._Split(s, windows=True)
        # Backslashes are literal path separators.
        self.assertEqual(split(r'C:\tools\diff.exe -u'),
                         [r'C:\tools\diff.exe', '-u'])
        self.assertEqual(split(r'"C:\Program Files\Diff\diff.exe" -u'),
                         [r'C:\Program Files\Diff\diff.exe', '-u'])
        # Single quotes do not group.
        self.assertEqual(split("notepad 'a b'"), ['notepad', "'a", "b'"])

    def testSplitCommandLinePlatform(self):
        s = r'"C:\a b\c.exe" \x'
        self.assertEqual(toolPaths.SplitCommandLine(s),
                         self._Split(s, sys.platform == 'win32'))


if __name__ == '__main__':
    unittest.main()
