#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
import json
import os
import subprocess
import sys
import tempfile
import textwrap
import unittest

class TestUsdInstall(unittest.TestCase):
    '''
    Test that a pip installed USD is functioning.
    '''

    def test_import(self):
        '''
        Test that we can import USD libraries.
        '''
        from pxr import Usd

    @unittest.skipUnless(sys.platform == 'win32', 'Windows DLL search paths')
    def test_windows_dll_paths(self):
        '''Test DLL search paths in fresh processes before importing pxr.'''
        with tempfile.TemporaryDirectory() as tempDir:
            paths = [os.path.join(tempDir, name) for name in
                     ('custom one', 'custom two', 'path fallback')]
            for path in paths:
                os.mkdir(path)

            for override in (os.pathsep.join(paths[:2]), '', None):
                with self.subTest(override=override):
                    # Set the override in the child so that an explicitly empty
                    # value remains distinct from an unset environment variable.
                    result = subprocess.run(
                        [sys.executable, '-c', textwrap.dedent('''
                            import json
                            import os
                            import sys
                            from unittest import mock

                            override = json.loads(sys.argv[1])
                            if override is None:
                                os.environ.pop('PXR_USD_WINDOWS_DLL_PATH', None)
                            else:
                                os.environ['PXR_USD_WINDOWS_DLL_PATH'] = (
                                    override)
                            originalPath = os.environ.get('PATH', '')

                            import pxr
                            dllPath = os.path.realpath(pxr.__file__)
                            dllPath = os.path.dirname(dllPath)
                            searchPath = (originalPath if override is None
                                          else override)
                            expected = os.pathsep.join(
                                path for path in (dllPath, searchPath) if path)
                            actual = os.environ['PXR_USD_WINDOWS_DLL_PATH']
                            assert actual == expected, (actual, expected)
                            assert os.environ['PATH'] == (
                                dllPath + os.pathsep + originalPath)

                            with mock.patch(
                                    'os.add_dll_directory',
                                    wraps=os.add_dll_directory) as addDir:
                                from pxr import Tf
                            registered = [call.args[0]
                                          for call in addDir.call_args_list]
                            assert dllPath in registered
                            for path in searchPath.split(os.pathsep):
                                if os.path.exists(path) and path != '.':
                                    assert os.path.abspath(path) in registered
                            if override is not None:
                                assert sys.argv[2] not in registered
                        '''), json.dumps(override), paths[2]],
                        env=dict(os.environ, PATH=paths[2]),
                        capture_output=True, text=True, timeout=60)
                    self.assertEqual(result.returncode, 0,
                                     result.stdout + result.stderr)

    @unittest.skipUnless(sys.platform == 'win32', 'Windows DLL search paths')
    def test_windows_dll_path_duplicates(self):
        '''Test prepend-once behavior, reloads, and missing/empty variables.'''
        result = subprocess.run(
            [sys.executable, '-c', textwrap.dedent('''
                import importlib
                import os
                import pxr

                dllPath = os.path.dirname(os.path.realpath(pxr.__file__))
                # Spell the wheel directory with different case, separators,
                # a trailing separator, and a redundant dot component.
                alias = dllPath.upper().replace(os.sep, os.altsep) + os.altsep
                otherPaths = ['C:/custom one', 'C:/custom two',
                              'C:/custom one']
                duplicatePaths = os.pathsep.join([
                    otherPaths[0], alias, otherPaths[1],
                    os.path.join(dllPath, '.'), '', otherPaths[2], dllPath])
                normalizedPaths = os.pathsep.join([dllPath] + otherPaths)
                names = ('PATH', 'PXR_USD_WINDOWS_DLL_PATH')

                for path in (None, '', duplicatePaths):
                    for override in (None, '', duplicatePaths):
                        for name, value in zip(names, (path, override)):
                            if value is None:
                                os.environ.pop(name, None)
                            else:
                                os.environ[name] = value

                        expectedPath = normalizedPaths if path else dllPath
                        expectedOverride = (
                            expectedPath if override is None else
                            normalizedPaths if override else dllPath)
                        importlib.reload(pxr)
                        actual = tuple(os.environ[name] for name in names)
                        expected = (expectedPath, expectedOverride)
                        assert actual == expected, (path, override, actual)

                        importlib.reload(pxr)
                        reloaded = tuple(os.environ[name] for name in names)
                        assert reloaded == actual
            ''')], capture_output=True, text=True, timeout=60)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_plugin_discovery(self):
        '''
        Test that we can discover plugins. If pluginfos are not being
        located, or if they refer to binary paths that can't be found,
        this simple test will fail. In that case, setting TF_DEBUG to
        PLUG* can be a very helpful first step in debugging.
        '''
        from pxr import Usd
        stage = Usd.Stage.CreateInMemory()
        self.assertTrue(stage is not None)

    def test_basic_script(self):
        '''
        Test that a general USD script runs as expected.
        '''
        from pxr import Sdf, Usd, UsdGeom
        stage = Usd.Stage.CreateInMemory()
        xformPrim = UsdGeom.Xform.Define(stage, '/hello')
        spherePrim = UsdGeom.Sphere.Define(stage, '/hello/world')
        self.assertEqual(spherePrim.GetPath(), Sdf.Path('/hello/world'))
        self.assertEqual(xformPrim.GetPath(), Sdf.Path('/hello'))
        self.assertIn('Xform', stage.ExportToString())
        self.assertGreater(spherePrim.GetRadiusAttr().Get(), 0)

