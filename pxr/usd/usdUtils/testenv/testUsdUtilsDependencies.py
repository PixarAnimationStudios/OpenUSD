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
from pathlib import Path
import os
import unittest

class TestUsdUtilsDependencies(unittest.TestCase):
    def test_ComputeAllDependencies(self):
        """Basic test for UsdUtils.ComputeAllDependencies"""

        def _testLayer(rootLayer, processingFunc):
            layers, assets, unresolved = \
                UsdUtils.ComputeAllDependencies(rootLayer, processingFunc)

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

        def _test(rootLayer, processingFunc = None):
            _testLayer(rootLayer, processingFunc)

            layer = Sdf.Layer.FindOrOpen(rootLayer)
            layer.SetPermissionToEdit(False)
            _testLayer(rootLayer, processingFunc)

        _test("computeAllDependencies/ascii.usda")
        _test("computeAllDependencies/ascii.usd")
        _test("computeAllDependencies/crate.usdc")
        _test("computeAllDependencies/crate.usd")

        # test identity processing func
        _test("computeAllDependencies/ascii.usda", lambda _, info: info)

    def test_ComputeAllDependenciesInvalidClipTemplate(self):
        """Test that an invalid clip template asset path does not
        cause an exception in UsdUtils.ComputeAllDependencies."""
        stage = Usd.Stage.CreateNew('testInvalidClipTemplate.usda')
        prim = stage.DefinePrim('/clip')
        Usd.ClipsAPI(prim).SetClipTemplateAssetPath('bogus')
        stage.Save()
        UsdUtils.ComputeAllDependencies(stage.GetRootLayer().identifier)

    def test_ComputeAllDependenciesUdims(self):
        """Test for catching UDIMs with UsdUtils.ComputeAllDependencies.
        Not included in the main test to keep it cleaner."""

        rootLayer = "computeAllDependenciesUdims/layer.usda"
        layers, assets, unresolved = \
            UsdUtils.ComputeAllDependencies(rootLayer)

        self.assertEqual(
            set(layers), 
            set([Sdf.Layer.Find(rootLayer)]))

        self.assertEqual(
            set([os.path.normcase(f) for f in assets]),
            set([os.path.normcase(
                    os.path.abspath("computeAllDependenciesUdims/" + f))
                    for f in ["image_a.1001.exr", 
                            "image_b.1002.exr", 
                            "image_c.1003.exr"]]))

        self.assertEqual(
            set([os.path.normcase(f) for f in unresolved]),
            set([os.path.normcase(
                    os.path.abspath("computeAllDependenciesUdims/" + f))
                    for f in ["image_d.<UDIM>.exr",
                            "image_e.<UDIM>.exr"]]))

    def test_ComputeAllDependenciesAnonymousLayer(self):
        """Test for resolving dependencies of anonymous layers"""
        layer = Sdf.Layer.CreateAnonymous(".usda")
        Sdf.Layer.CreateAnonymous(".usda").Export("anon_sublayer.usda")

        layer.subLayerPaths.append("anon_sublayer.usda")
        layer.subLayerPaths.append("unresolved.usda")

        layers, _, unresolved = UsdUtils.ComputeAllDependencies(layer.identifier)
        self.assertEqual(layers, [
            Sdf.Layer.Find(layer.identifier), 
            Sdf.Layer.Find("anon_sublayer.usda")])
        self.assertEqual(unresolved, ["unresolved.usda"])

    def test_ComputeAllDependenciesUserFuncFilterPaths(self):
        """Tests paths that are filtered by the processing func 
        do not appear in results"""

        stagePath = "test_filter.usda"
        assetDirPath = "./asset_dep_dir"
        assetFilePath = "./non_dir_dep.usda"

        if not os.path.exists(assetDirPath): os.mkdir(assetDirPath)
        assetDepLayer = Sdf.Layer.CreateNew(assetFilePath)
        assetDepLayer.Save()

        stage = Usd.Stage.CreateNew(stagePath)
        prim = stage.DefinePrim("/test")
        dirAttr = prim.CreateAttribute("dirAsset", Sdf.ValueTypeNames.Asset)
        dirAttr.Set(assetDirPath)
        nonDirAttr = prim.CreateAttribute("depAsset", Sdf.ValueTypeNames.Asset)
        nonDirAttr.Set(assetFilePath)
        stage.GetRootLayer().Save()

        def FilterDirectories(layer, depInfo):
            if (os.path.isdir(depInfo.assetPath)):
                return UsdUtils.DependencyInfo()
            else:
                return depInfo

        layers, references, unresolved = \
            UsdUtils.ComputeAllDependencies(stagePath, FilterDirectories)
        
        self.assertEqual(layers, [stage.GetRootLayer(), assetDepLayer])
        self.assertEqual(references, [])
        self.assertEqual(unresolved, [])

    def test_ComputeAllDependenciesUserFuncAdditionalPaths(self):
        """Tests additional paths that are specified by the user processing func
        appear in results"""

        stagePath = "test_additional_deps.usda"
        assetPath = "additional_dep.txt"
        assetPathDep = "additional_dep.txt2"
        Path(assetPath).touch()
        Path(assetPathDep).touch()
        stage = Usd.Stage.CreateNew(stagePath)
        prim = stage.DefinePrim("/test")
        attr = prim.CreateAttribute("depAsset", Sdf.ValueTypeNames.Asset)
        attr.Set(assetPath)
        stage.GetRootLayer().Save()

        def AddAdditionalDeps(layer, depInfo):
            return UsdUtils.DependencyInfo(
                depInfo.assetPath, [depInfo.assetPath + "2"])


        layers, references, unresolved = \
            UsdUtils.ComputeAllDependencies(stagePath, AddAdditionalDeps)
        
        self.assertEqual(layers, [stage.GetRootLayer()])
        self.assertEqual([os.path.normcase(f) for f in references], 
                         [os.path.normcase(os.path.abspath(assetPath)), 
                          os.path.normcase(os.path.abspath(assetPathDep))])
        self.assertEqual(unresolved, [])

    def test_ComputeAllDependenciesUserFuncModifyPathss(self):
        """Tests assets paths which are modified by the processing func
        appear correctly in returned vectors"""

        stagePath = "test_modified_deps.usda"
        assetPath = "modified_dep.txt"
        Path(assetPath).touch()
        stage = Usd.Stage.CreateNew(stagePath)
        prim = stage.DefinePrim("/test")
        attr = prim.CreateAttribute("depAsset", Sdf.ValueTypeNames.Asset)
        attr.Set("dep.txt")
        stage.GetRootLayer().Save()

        def ModifyDeps(layer, depInfo):
            return UsdUtils.DependencyInfo("modified_" + depInfo.assetPath)


        layers, references, unresolved = \
            UsdUtils.ComputeAllDependencies(stagePath, ModifyDeps)
        
        self.assertEqual(layers, [stage.GetRootLayer()])
        self.assertEqual([os.path.normcase(f) for f in references],
            [os.path.normcase(os.path.abspath(assetPath))])
        self.assertEqual(unresolved, [])

    def test_ComputeAllDependenciesParseAdditionalLayers(self):
        """Tests that layers that are specified as additional dependencies are
        themselves processed for additional assets"""

        def CreateStageWithDep(stagePath, depPath):
            stage = Usd.Stage.CreateNew(stagePath)
            prim = stage.DefinePrim("/test")
            if depPath is not None:
                attr = prim.CreateAttribute("depAsset", Sdf.ValueTypeNames.Asset)
                attr.Set(depPath)
            stage.GetRootLayer().Save()
            return stage
        
        stagePath = "test_process_deps.usda"
        asset = CreateStageWithDep(stagePath, "dep.usda")
        dep = CreateStageWithDep("dep.usda", None)
        extra = CreateStageWithDep("extra_dep.usda", "file.txt")
        Path("file.txt").touch()

        def AddExtra(layer, depInfo):
            if depInfo.assetPath.startswith("dep"):
                return UsdUtils.DependencyInfo(
                    depInfo.assetPath, ["extra_" + depInfo.assetPath])
            else:
                return depInfo
            
        layers, references, unresolved = \
            UsdUtils.ComputeAllDependencies(stagePath, AddExtra)
        
        self.assertEqual(layers, [asset.GetRootLayer(), dep.GetRootLayer(), 
                                  extra.GetRootLayer()])
        self.assertEqual([os.path.normcase(f) for f in references],
            [os.path.normcase(os.path.abspath("file.txt"))])
        self.assertEqual(unresolved, [])
            


if __name__=="__main__":
    unittest.main()
