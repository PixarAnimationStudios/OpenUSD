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

from pxr import Usd, UsdGeom, UsdLux, Vt, Sdf
import unittest

testFile = "linking_example.usda"
stage = Usd.Stage.Open(testFile)

class TestUsdLuxLinkingAPI(unittest.TestCase):
    def test_LinkageQueries(self):
        test_cases = (
            ('/Lights/DefaultLinkage/include_all', '/Geom', True),
            ('/Lights/DefaultLinkage/include_all', '/Geom/a', True),
            ('/Lights/DefaultLinkage/include_all', '/Geom/a/sub_scope', True),

            ('/Lights/SimpleInclude/include_a', '/Geom', False),
            ('/Lights/SimpleInclude/include_a', '/Geom/a', True),
            ('/Lights/SimpleInclude/include_a', '/Geom/a/sub_scope', True),
            ('/Lights/SimpleInclude/include_a', '/Geom/b', False),

            ('/Lights/SimpleExclude/exclude_a', '/Geom', True),
            ('/Lights/SimpleExclude/exclude_a', '/Geom/a', False),
            ('/Lights/SimpleExclude/exclude_a', '/Geom/a/sub_scope', False),
            ('/Lights/SimpleExclude/exclude_a', '/Geom/b', True),

            ('/Lights/FaceSetLinking/include_faceSet_example', '/', False),
            ('/Lights/FaceSetLinking/include_faceSet_example', '/Geom', False),
            ('/Lights/FaceSetLinking/include_faceSet_example', '/Geom/meshWithFaceSet', False),
            ('/Lights/FaceSetLinking/include_faceSet_example', '/Geom/meshWithFaceSet/faceSet', True),
        )
        for light_path, test_path, expected_result in test_cases:
            light = UsdLux.Light(stage.GetPrimAtPath(light_path))
            links = light.GetLightLinkCollectionAPI()
            query = links.ComputeMembershipQuery()
            actual_result = query.IsPathIncluded(test_path)
            self.assertEqual(actual_result, expected_result)

    def test_LinkageAuthoring(self):
        stage = Usd.Stage.CreateInMemory()
        geom_scope = UsdGeom.Scope.Define(stage, '/Geom')
        sphere = UsdGeom.Sphere.Define(stage, '/Geom/Sphere')
        light_scope = UsdGeom.Scope.Define(stage, '/Lights')
        light_1 = UsdLux.SphereLight.Define(stage, '/Lights/light_1')
        light_1_links = UsdLux.Light(light_1).GetLightLinkCollectionAPI()

        # Schema default: link everything
        query = light_1_links.ComputeMembershipQuery()
        self.assertTrue(query.IsPathIncluded(geom_scope.GetPath()))
        self.assertTrue(query.IsPathIncluded(sphere.GetPath()))
        self.assertTrue(query.IsPathIncluded('/RandomOtherPath'))

        # Exclude /Geom.
        light_1_links.ExcludePath('/Geom')
        query = light_1_links.ComputeMembershipQuery()
        self.assertFalse(query.IsPathIncluded(geom_scope.GetPath()))
        self.assertFalse(query.IsPathIncluded(sphere.GetPath()))
        self.assertTrue(query.IsPathIncluded('/RandomOtherPath'))

        # Include /Geom/Sphere
        light_1_links.IncludePath('/Geom/Sphere')
        query = light_1_links.ComputeMembershipQuery()
        self.assertFalse(query.IsPathIncluded(geom_scope.GetPath()))
        self.assertTrue(query.IsPathIncluded(sphere.GetPath()))
        self.assertTrue(query.IsPathIncluded('/Geom/Sphere/Child'))
        self.assertTrue(query.IsPathIncluded('/RandomOtherPath'))

    def test_FilterLinking(self):
        light_path = '/Lights/FilterLinking/filter_exclude_a'
        light = UsdLux.Light(stage.GetPrimAtPath(light_path))
        filter_paths = light.GetFiltersRel().GetForwardedTargets()
        self.assertEqual(len(filter_paths), 1)
        light_filter = UsdLux.LightFilter(stage.GetPrimAtPath(filter_paths[0]))
        self.assertEqual(light_filter.GetPath(), '/Lights/FilterLinking/filter')
        links = light_filter.GetFilterLinkCollectionAPI()
        query = links.ComputeMembershipQuery()
        self.assertFalse(query.IsPathIncluded('/Geom/a'))
        self.assertTrue(query.IsPathIncluded('/Geom/b'))

if __name__ == "__main__":
    unittest.main()
