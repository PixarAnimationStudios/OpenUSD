#!/pxrpythonsubst
#
# Copyright 2023 Pixar
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
from pxr import UsdUtils, Plug, Sdf

import os, unittest


class TestUsdUtilsDependenciesCustomResolver(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Register test resolver plugins
        # Test plugins are installed relative to this script
        testRoot = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), 'UsdUtilsPlugins')

        pr = Plug.Registry()

        testURIResolverPath = os.path.join(
            testRoot, 'lib/TestUsdUtilsDependenciesCustomResolver*/Resources/')
        pr.RegisterPlugins(testURIResolverPath)

    def test_ComputeAllDependencies(self):
        """Tests that ComputeAllDependencies correctly sets identifier and """
        """resolved paths when using a resolver with multiple uri schemes """

        mainIdentifier = "test:main.usda"
        expectedMainResolvedPath = "testresolved:main.usda"
        dependencyIdentifier = "test:dependency.usda"
        expectedDependencyResolvedPath = "testresolved:dependency.usda"

        layers, _, _ = UsdUtils.ComputeAllDependencies(mainIdentifier)

        self.assertEqual(len(layers), 2)
        layer0 = layers[0]
        self.assertEqual(layer0.identifier, mainIdentifier)
        self.assertEqual(layer0.resolvedPath, expectedMainResolvedPath)

        layer1 = layers[1]
        self.assertEqual(layer1.identifier, dependencyIdentifier)
        self.assertEqual(layer1.resolvedPath, expectedDependencyResolvedPath)

    def test_findOrOpenLayer(self):
        mainIdentifier = "test:main.usda"
        expectedMainResolvedPath = "testresolved:main.usda"
        dependencyIdentifier = "test:dependency.usda"
        expectedDependencyResolvedPath = "testresolved:dependency.usda"

        UsdUtils.ComputeAllDependencies(mainIdentifier)

        layer = Sdf.Layer.FindOrOpen(mainIdentifier)
        self.assertEqual(layer.identifier, mainIdentifier)
        self.assertEqual(layer.resolvedPath, expectedMainResolvedPath)

        layer = Sdf.Layer.FindOrOpen(dependencyIdentifier)
        self.assertEqual(layer.identifier, dependencyIdentifier)
        self.assertEqual(layer.resolvedPath, expectedDependencyResolvedPath)


if __name__=="__main__":
    unittest.main()
