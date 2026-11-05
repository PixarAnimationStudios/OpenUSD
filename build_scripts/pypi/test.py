#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
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

    def test_wheel_abi_tag(self):
        '''
        Test that the installed wheel is tagged with a specific Python ABI
        rather than "none", so that installers reject it on interpreters
        with an incompatible ABI, such as free-threaded builds.
        '''
        from importlib.metadata import distribution
        wheelMetadata = distribution('usd-core').read_text('WHEEL')
        self.assertIsNotNone(wheelMetadata,
                             'usd-core does not appear to be installed from a '
                             'wheel')
        tags = [line.split(':', 1)[1].strip()
                for line in wheelMetadata.splitlines()
                if line.startswith('Tag:')]
        self.assertTrue(tags)
        for tag in tags:
            interpreter, abi, platform = tag.split('-', 2)
            self.assertNotEqual(abi, 'none', tag)
            
