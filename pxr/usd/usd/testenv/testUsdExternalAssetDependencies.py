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

from __future__ import print_function

from pxr import Plug, Sdf, Tf, Usd
import os, unittest, shutil

class TestUsdExternalAssetDependencies(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        testRoot = os.path.join(os.path.dirname(__file__), 'UsdPlugins')
        testPluginsDso = testRoot + '/lib'
        testPluginsDsoSearch = testPluginsDso + '/*/Resources/'

        # Register dso plugins.  Discard possible exception due to TestPlugDsoEmpty.
        # The exception only shows up here if it happens in the main thread so we
        # can't rely on it.
        try:
            Plug.Registry().RegisterPlugins(testPluginsDsoSearch)
        except RuntimeError:
            pass

    def setUp(self):
        # Our test creates a new file in the run directory that unfortunately
        # doesn't get cleaned up automatically on subsequent runs on local
        # machines. Make sure this file doesn't exist before starting our
        # test case.
        if os.path.isfile("sphere_copy.usda"):
            os.remove("sphere_copy.usda")
        self.assertFalse(os.path.exists("sphere_copy.usda"))

    def test_ExternalAssetDependencies(self):

        # Open the procedural test file first, just to verify the test file
        # format is working.
        layerFile = 'root.test_usd_pea'
        layer = Sdf.Layer.FindOrOpen(layerFile)
        self.assertTrue(layer)

        # The layer file format generates its contents from the other valid 
        # layer files in the same directory. There should be two of these layer
        # files and their paths will be returned by 
        # GetExternalAssetDependencies.
        self.assertEqual(layer.GetExternalAssetDependencies(), 
                         [layer.ComputeAbsolutePath("cubes.usda"),
                          layer.ComputeAbsolutePath("sphere.usda")])

        # Create a stage for this layer. Verify that the only layers that are
        # used by the stage are this layer and the session layer. The external
        # dependencies do not remain open as a stage layer.
        stage = Usd.Stage.Open(layer, None)
        self.assertTrue(stage)
        self.assertEqual(len(stage.GetUsedLayers()), 2)
        self.assertIn(layer, stage.GetUsedLayers())
        self.assertIn(stage.GetSessionLayer(), stage.GetUsedLayers())

        # Context wrapper for verifying whether the above stage has had its
        # contents changed or not changed as expected after an operation. It
        # also verifies that the stage has the expected prims afterwards.
        class VerifyStageChange:
            _test = self

            # Initialized with whether a content change is expected and the 
            # paths of the expected stage prims.
            def __init__(self, contentsShouldChange, expectedPrimPaths):
                self.contentsShouldChange = contentsShouldChange
                self.expectedPrimPaths = expectedPrimPaths
                # Register a listener for the contents changed notice on 
                # the stage.
                self.listener = Tf.Notice.Register(
                    Usd.Notice.StageContentsChanged,
                    self.OnStageContentsChanged, stage)

            def __enter__(self):
                # Reset the changed flag
                self.contentsChanged = False
                return self

            def __exit__(self, *args):
                # On exit, verify that the contents change notice was sent or
                # not sent as expected and verify the stage has all the expected
                # prims
                self._test.assertEqual(
                    self.contentsChanged, self.contentsShouldChange)
                self._test.assertEqual(
                    list(stage.TraverseAll()), 
                    [stage.GetPrimAtPath(p) for p in self.expectedPrimPaths])

            def OnStageContentsChanged(self, *args):
                # Flag notice received.
                self.contentsChanged = True

        # Initial expected prims. Verify that the test stage starts with these
        # prims.
        expectedPrims = ["/cubes_usda", 
                         "/cubes_usda/First", 
                         "/sphere_usda",
                         "/sphere_usda/Geom"]
        self.assertEqual(
            list(stage.TraverseAll()), 
            [stage.GetPrimAtPath(p) for p in expectedPrims])

        # Sanity check. Immediate stage reload does nothing as nothing's 
        # changed.
        with VerifyStageChange(False, expectedPrims):
            stage.Reload()

        # Sanity check. Force reloading the root layer does trigger a stage
        # change notice.
        with VerifyStageChange(True, expectedPrims):
            layer.Reload(force=True)

        # Test case. Add a copy of the sphere file to the same directory. This
        # adds another external asset dependency to the procedural root layer.
        # But verify the stage does NOT change as the layer does not trigger
        # any change notifications for external assets
        with VerifyStageChange(False, expectedPrims):
            shutil.copyfile(layer.ComputeAbsolutePath("sphere.usda"), 
                            layer.ComputeAbsolutePath("sphere_copy.usda"))
            self.assertEqual(layer.GetExternalAssetDependencies(), 
                             [layer.ComputeAbsolutePath("cubes.usda"),
                              layer.ComputeAbsolutePath("sphere.usda"),
                              layer.ComputeAbsolutePath("sphere_copy.usda")])

        # Now reload the stage. The stage will reload with the root layer 
        # regenerate its contents to include the new content from sphere_copy.
        expectedPrims = ["/cubes_usda", 
                         "/cubes_usda/First", 
                         "/sphere_usda",
                         "/sphere_usda/Geom",
                         "/sphere_copy_usda",
                         "/sphere_copy_usda/Geom"]
        with VerifyStageChange(True, expectedPrims):
            stage.Reload()

        # Verify reloading the stage again causes no changes
        with VerifyStageChange(False, expectedPrims):
            stage.Reload()

        # Now open the cubes layer (which will not already be open), change
        # its default prim, and save. This will cause the contents of the root
        # layer to change when it is reloaded but nothing gets reloaded yet
        # as the cubes layer is an external dependency that is not open for 
        # the stage.
        with VerifyStageChange(False, expectedPrims):
            self.assertFalse(Sdf.Layer.Find("cubes.usda"))
            cubesLayer = Sdf.Layer.FindOrOpen("cubes.usda")
            self.assertTrue(cubesLayer)
            cubesLayer.defaultPrim = "Cube2"
            cubesLayer.Save()

        # Now reload tha stage. The root layer will be regenerated because one
        # its external asset dependencies has changed and the new prims will
        # be present.
        expectedPrims = ["/cubes_usda", 
                         "/cubes_usda/Second", 
                         "/sphere_usda",
                         "/sphere_usda/Geom",
                         "/sphere_copy_usda",
                         "/sphere_copy_usda/Geom"]
        with VerifyStageChange(True, expectedPrims):
            stage.Reload()

if __name__ == "__main__":
    unittest.main()
