#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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
from pxr import UsdShade

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    appController._stageView.updateGL()
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Update mesh binding.
def _updateMeshBinding(path, appController):
    s = appController._dataModel.stage
    l = s.GetSessionLayer()
    s.SetEditTarget(l)

    mesh = s.GetPrimAtPath('/Scene/Geom/Plane')
    material = UsdShade.Material(s.GetPrimAtPath(path))
    UsdShade.MaterialBindingAPI(mesh).Bind(material)

# Test material bindings edits.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _updateMeshBinding('/Scene/Looks/MainMaterial_1', appController)
    _takeShot(appController, "1.png")
    _updateMeshBinding('/Scene/Looks/MainMaterial_2', appController)
    _takeShot(appController, "2.png")
    _updateMeshBinding('/Scene/Looks/MainMaterial_1', appController)
    _takeShot(appController, "3.png")
