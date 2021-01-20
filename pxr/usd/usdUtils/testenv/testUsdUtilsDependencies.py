#!/pxrpythonsubst
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

from pxr import UsdUtils, Sdf, Usd
import os
import unittest

class TestUsdUtilsDependencies(unittest.TestCase):
    def test_ComputeAllDependencies(self):
        """Basic test for UsdUtils.ComputeAllDependencies"""

        def _test(rootLayer):
            layers, assets, unresolved = \
                UsdUtils.ComputeAllDependencies(rootLayer)

            self.assertEqual(
               set(layers),
               set([Sdf.Layer.Find(rootLayer)] + 
                   [Sdf.Layer.Find("computeAllDependencies/" + l)
                    for l in ["base_a.usd",
                              "sub_a.usd",
                              "meta_a.usd",
                              "meta_b.usd",
                              "payload_a.usd",
                              "attr_b.usd",
                              "ref_a.usd",
                              "v_meta_a.usd",
                              "v_meta_b.usd",
                              "v_ref_a.usd",
                              "v_attr_b.usd",
                              "clip.1.usd",
                              "clip.010.usd"]]))

            # Canonicalize all paths being compared with os.path.normcase
            # to avoid issues on Windows.
            
            self.assertEqual(
                set([os.path.normcase(f) for f in assets]),
                set([os.path.normcase(
                        os.path.abspath("computeAllDependencies/" + f))
                     for f in ["attr_a.txt", "v_attr_a.txt"]]))

            self.assertEqual(
                set([os.path.normcase(f) for f in unresolved]),
                set([os.path.normcase(
                        os.path.abspath("computeAllDependencies/" + f) )
                     for f in ["base_nonexist.usd",
                               "sub_nonexist.usd",
                               "meta_a_nonexist.usd",
                               "meta_nonexist.usd",
                               "payload_nonexist.usd",
                               "attr_a_nonexist.txt",
                               "attr_nonexist.usd",
                               "ref_nonexist.usd",
                               "v_meta_a_nonexist.usd",
                               "v_meta_nonexist.usd",
                               "v_attr_a_nonexist.txt",
                               "v_attr_nonexist.usd"]]))

        _test("computeAllDependencies/ascii.usda")
        _test("computeAllDependencies/ascii.usd")
        _test("computeAllDependencies/crate.usdc")
        _test("computeAllDependencies/crate.usd")

    def test_ComputeAllDependenciesInvalidClipTemplate(self):
        """Test that an invalid clip template asset path does not
        cause an exception in UsdUtils.ComputeAllDependencies."""
        stage = Usd.Stage.CreateNew('testInvalidClipTemplate.usda')
        prim = stage.DefinePrim('/clip')
        Usd.ClipsAPI(prim).SetClipTemplateAssetPath('bogus')
        stage.Save()
        UsdUtils.ComputeAllDependencies(stage.GetRootLayer().identifier)

if __name__=="__main__":
    unittest.main()
