#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Gf, Tf, Kind, Plug
import os, unittest, shutil

class TestKindRegistry(unittest.TestCase):
    def test_Basic(self):
        # Create testing environment
        testRoot = os.path.join(os.path.dirname(__file__), 'KindPlugins')
        testPluginsPython = testRoot + '/lib/python'
        targetDirectory = testPluginsPython+'/TestKindModule'
    
        if not os.path.exists(targetDirectory):
            os.makedirs(targetDirectory)
            shutil.copyfile('TestKindModule_plugInfo.json', 
                            os.path.join(targetDirectory, 'plugInfo.json'))
            shutil.copyfile('TestKindModule__init__.py',
                            os.path.join(targetDirectory, '__init__.py'))

        # Register python module plugins
        Plug.Registry().RegisterPlugins(testPluginsPython + "/**/")

        reg = Kind.Registry()
        self.assertTrue(reg)

        # Test factory default kinds + config file contributions
        expectedDefaultKinds = [
            'group',
            'model',
            'test_model_kind',
            'test_root_kind',
            ]
        actualDefaultKinds = Kind.Registry.GetAllKinds()

        # We cannot expect actual to be equal to expected, because there is no
        # way to prune the site's extension plugins from actual.
        # assertEqual(sorted(expectedDefaultKinds), sorted(actualDefaultKinds))

        for expected in expectedDefaultKinds:
            self.assertTrue( Kind.Registry.HasKind(expected) )
            self.assertTrue( expected in actualDefaultKinds )

        # Check the 'test_model_kind' kind from the TestKindModule_plugInfo.json
        self.assertTrue(Kind.Registry.HasKind('test_root_kind'))
        self.assertEqual(Kind.Registry.GetBaseKind('test_root_kind'), '')

        # Check the 'test_model_kind' kind from the TestKindModule_plugInfo.json
        self.assertTrue(Kind.Registry.HasKind('test_model_kind'))
        self.assertEqual(Kind.Registry.GetBaseKind('test_model_kind'), 'model')

if __name__ == "__main__":
    unittest.main()
