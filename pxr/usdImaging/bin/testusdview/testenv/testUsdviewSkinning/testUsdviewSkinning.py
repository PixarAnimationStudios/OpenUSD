#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from pxr.UsdAppUtils.complexityArgs import RefinementComplexities
from pxr.Usdviewq.common import RenderModes, Usd, UsdGeom
from pxr import Vt, Gf


# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

    # Make image differences clear.
    appController._dataModel.viewSettings.renderMode = RenderModes.WIREFRAME

# Set the complexity and refresh the view.
def _setComplexity(appController, complexity):
    appController._dataModel.viewSettings.complexity = complexity
    appController._stageView.updateView()

def _testChangeComplexity(appController):
    _setComplexity(appController, RefinementComplexities.MEDIUM)
    appController._takeShot("change_complexity.png")
    _setComplexity(appController, RefinementComplexities.LOW)

def _testInvisVisOnPlayback(appController):
    # Start playback
    appController.setFrame(4)
    appController._stageView.updateView()
    appController._takeShot("vis_frame_4.png")

    stage = appController._dataModel.stage
    skelRoot = UsdGeom.Imageable(stage.GetPrimAtPath("/Model"))
    print("Invising skel root.")
    skelRoot.MakeInvisible()
    appController._stageView.updateView()
    appController._takeShot("invis_frame_4.png")
    
    print("Scrubbing a few frames ahead.")
    appController.setFrame(5)
    appController.setFrame(6)
    appController.setFrame(7)

    print("Vising skel root.")
    skelRoot.MakeVisible()
    appController.setFrame(8)
    appController._takeShot("vis_frame_8.png")

# Force the skinned prim to resync by modifying a built-in primvar (such
# as displayColor).
def _testResyncSkinnedPrim(appController):
    appController._dataModel.viewSettings.renderMode = RenderModes.FLAT_SHADED
    appController.setFrame(2)
    appController._takeShot("pre_skinned_prim_resync_frame_2.png")

    stage = appController._dataModel.stage
    arm = stage.GetPrimAtPath("/Model/Arm")
    attr = arm.GetAttribute('primvars:displayColor')
    # Changing the display color should trigger the resync
    attr.Set(Vt.Vec3fArray(1, Gf.Vec3f(1.0,0.0,0.0)))

    appController._stageView.updateView()
    appController._takeShot("post_skinned_prim_resync_frame_2.png")

# Force the skeleton prim to resync by modifying its rest xform.
def _testResyncSkeleton(appController):
    appController._dataModel.viewSettings.renderMode = RenderModes.FLAT_SHADED
    appController.setFrame(6)
    appController._takeShot("pre_skel_resync_frame_6.png")

    stage = appController._dataModel.stage
    arm = stage.GetPrimAtPath("/Model/Skel")
    attr = arm.GetAttribute('restTransforms')
    # Changing the rest xform should trigger the resync
    attr.Set(Vt.Matrix4dArray(3, (Gf.Matrix4d(1.0, 0.0, 0.0, 0.0,
                                              0.0, 1.0, 0.0, 0.0,
                                              0.0, 0.0, 1.0, 0.0,
                                              1.0, 2.0, 3.0, 1.0),
                                  Gf.Matrix4d(1.0, 0.0, 0.0, 0.0,
                                              0.0, 1.0, 0.0, 0.0,
                                              0.0, 0.0, 1.0, 0.0,
                                              3.0, 4.0, 3.0, 1.0),
                                  Gf.Matrix4d(1.0, 0.0, 0.0, 0.0,
                                              0.0, 1.0, 0.0, 0.0,
                                              0.0, 0.0, 1.0, 0.0,
                                              2.0, 0.0, 2.0, 1.0))))

    appController._stageView.updateView()
    appController._takeShot("post_skel_resync_frame_6.png")

# Skinning makes use of adapter hijacking (for the skinned prims). Callbacks
# for various operations need to be forwarded/processed correctly.
# This test attempts to capture these scenarios.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testChangeComplexity(appController)
    _testInvisVisOnPlayback(appController)
    _testResyncSkinnedPrim(appController)
    _testResyncSkeleton(appController)
