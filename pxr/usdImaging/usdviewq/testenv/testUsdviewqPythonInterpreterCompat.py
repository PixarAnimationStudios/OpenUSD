#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
"""
Standalone regression check for the two bugs reported in GitHub issue #4209
("usdview Python interpreter fails on Windows with Python 3.14"):

1. `Controller.ExecStartupFile` embedded the $PYTHONSTARTUP path into
   generated Python source inside plain double quotes (`"{0}"`.format(path)),
   so a Windows path's backslashes were read back as escape sequences by the
   generated source's own parser, breaking on paths such as
   C:\\Users\\name\\...\\pythonrc.py.
2. `Interpreter.showsyntaxerror` only accepted `filename`, but
   `code.InteractiveInterpreter.runsource` calls
   `self.showsyntaxerror(filename, source=source)` -- passing `source` as a
   keyword -- so the override raised TypeError on any syntax error.

`pythonInterpreter.py` can't be imported directly outside a built USD (it
imports `pxr.Tf` and PySide6 at module scope), so this script re-implements
each fixed snippet in isolation using only the stdlib and runs it for real
(not just reasoned about statically). It is a standalone script, not wired
into CMakeLists.txt/ctest -- there was no build environment available to add
and verify that wiring; a maintainer should consider registering it as a
usdviewq test if useful.

Run directly: python testUsdviewqPythonInterpreterCompat.py
"""

import unittest
from code import InteractiveInterpreter


def build_exec_startup_source(path):
    """Mirrors the fixed Controller.ExecStartupFile in pythonInterpreter.py."""
    return (
        'g = dict(globals());'
        'g["__file__"] = {0!r};'
        'f = open({0!r}, "rb");'
        'exec(compile(f.read(), {0!r}, "exec"), g);'
        'f.close();'
        'del g["__file__"];'
        'globals().update(g);'.format(path))


class _FixedInterpreter(InteractiveInterpreter):
    """Mirrors the fixed Interpreter.showsyntaxerror override."""

    def __init__(self):
        super().__init__()
        self.sawSyntaxError = False

    def showsyntaxerror(self, filename=None, **kwargs):
        self.sawSyntaxError = True
        # Real override wraps this in a Qt brush; irrelevant to the bug.
        super().showsyntaxerror(filename, **kwargs)

    def write(self, data):
        pass  # Suppress traceback printing during the test.


class _PreFixInterpreter(InteractiveInterpreter):
    """Mirrors the ORIGINAL (buggy) Interpreter.showsyntaxerror override."""

    def showsyntaxerror(self, filename=None):
        super().showsyntaxerror(filename)

    def write(self, data):
        pass


class TestExecStartupFilePathEmbedding(unittest.TestCase):
    def test_windows_path_compiles_after_fix(self):
        windows_path = r"C:\Users\Jonas\AppData\Roaming\Code\pythonrc.py"
        source = build_exec_startup_source(windows_path)
        # Must not raise SyntaxError -- this is exactly what broke before
        # the fix, with Windows paths like the one from the issue.
        compile(source, "<test>", "exec")

    def test_windows_path_with_backslash_u_breaks_naive_quoting(self):
        # Sanity-check that this reproduction is real: the OLD "{0}" (plain
        # double-quote) embedding is indeed broken for a path containing a
        # \U sequence (an 8-hex-digit unicode escape introducer in Python
        # string literals), which is exactly the class of path in the
        # issue's reproduction (\Users\...).
        windows_path = r"C:\Users\Jonas\pythonrc.py"
        old_style_source = (
            'g = dict(globals());'
            'g["__file__"] = "{0}";'
            'f = open("{0}", "rb");'
            'exec(compile(f.read(), "{0}", "exec"), g);'
            'f.close();'
            'del g["__file__"];'
            'globals().update(g);'.format(windows_path))
        with self.assertRaises(SyntaxError):
            compile(old_style_source, "<test>", "exec")


class TestShowSyntaxErrorSignature(unittest.TestCase):
    def test_fixed_interpreter_handles_syntax_error(self):
        interp = _FixedInterpreter()
        # runsource() internally calls
        # self.showsyntaxerror(filename, source=source) on a SyntaxError --
        # this must not raise TypeError.
        more = interp.runsource("this is not valid python !!!")
        self.assertFalse(more)
        self.assertTrue(interp.sawSyntaxError)

    def test_prefix_interpreter_raises_typeerror(self):
        # Mutation check: confirms this test suite actually catches the
        # reported regression against the ORIGINAL override.
        interp = _PreFixInterpreter()
        with self.assertRaises(TypeError):
            interp.runsource("this is not valid python !!!")


if __name__ == "__main__":
    unittest.main()
