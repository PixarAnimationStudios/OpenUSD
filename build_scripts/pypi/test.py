#
# Copyright 2020 Pixar
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

