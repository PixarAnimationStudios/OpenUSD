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


class TestUsdLuxListAPI(unittest.TestCase):
    def test_ListAPI(self):
        self._test( Usd.Stage.Open('root.usda',
            Usd.Stage.LoadNone) )
    def test_ListAPI(self):
        self._test( Usd.Stage.Open('root_with_instances.usda',
            Usd.Stage.LoadNone) )

    def _test(self, stage):
        listAPI = UsdLux.ListAPI(stage.GetPrimAtPath('/World'))
        consult = UsdLux.ListAPI.ComputeModeConsultModelHierarchyCache
        ignore = UsdLux.ListAPI.ComputeModeIgnoreCache

        # no cache initially
        self.assertEqual(len(listAPI.GetLightListRel().GetTargets()), 0)
        # compute w/o cache should find 1 light outside payload
        computed_list = listAPI.ComputeLightList(ignore)
        self.assertEqual(len(computed_list), 1)
        self.assertTrue(Sdf.Path('/World/Lights/Sky_light') in computed_list)
        # compute w/ cache should find 1 extra light, since 1 light
        # inside a payload has been published to cache
        computed_list = listAPI.ComputeLightList(consult)
        self.assertEqual(len(computed_list), 2)
        self.assertTrue(Sdf.Path('/World/Lights/Sky_light') in computed_list)
        self.assertTrue(Sdf.Path('/World/Geo/torch_2/light') in computed_list)

        # load payloads to discover the rest of the lights
        stage.Load()
        # if we consult the cache we still won't see lights below model
        # hierarchy
        computed_list = listAPI.ComputeLightList(consult)
        self.assertEqual(len(computed_list), 2)
        self.assertTrue(Sdf.Path('/World/Lights/Sky_light') in computed_list)
        self.assertTrue(Sdf.Path('/World/Geo/torch_2/light') in computed_list)
        # but if we ignore cache we now see 3 lights
        computed_list = listAPI.ComputeLightList(ignore)
        self.assertEqual(len(computed_list), 3)
        self.assertTrue(Sdf.Path('/World/Lights/Sky_light') in computed_list)
        self.assertTrue(Sdf.Path('/World/Geo/torch_1/light') in computed_list)
        self.assertTrue(Sdf.Path('/World/Geo/torch_2/light') in computed_list)

        # store this full list in the light list
        listAPI.StoreLightList(computed_list)

        # now using the cache should return everything
        computed_list = listAPI.ComputeLightList(consult)
        self.assertEqual(len(computed_list), 3)
        self.assertTrue(Sdf.Path('/World/Lights/Sky_light') in computed_list)
        self.assertTrue(Sdf.Path('/World/Geo/torch_1/light') in computed_list)
        self.assertTrue(Sdf.Path('/World/Geo/torch_2/light') in computed_list)

        # deactivate 1 torch model
        torch_1 = stage.GetPrimAtPath('/World/Geo/torch_1')
        torch_1.SetActive(False)

        # if we ignore the cache(s) we do see only 2 lights
        self.assertEqual(len(listAPI.ComputeLightList(ignore)), 2)
        # but the cache should continue to report 3 lights
        self.assertEqual(len(listAPI.ComputeLightList(consult)), 3)
        # invalidating the cache should cause it to report 2 lights
        listAPI.InvalidateLightList()
        self.assertEqual(len(listAPI.ComputeLightList(consult)), 2)

        # add a light filter, and confirm that it gets included as a light
        self.assertEqual(len(listAPI.ComputeLightList(ignore)), 2)
        filter = UsdLux.LightFilter.Define(stage, '/World/Lights/TestFilter')
        self.assertEqual(len(listAPI.ComputeLightList(ignore)), 3)

        # discard changes
        stage.Reload()

if __name__ == "__main__":
    unittest.main()
