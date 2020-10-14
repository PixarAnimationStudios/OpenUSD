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

from pxr.Usdviewq.freeCamera import FreeCamera
from pxr import Gf

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

def _GetFreeCamera(appController):
    stageView = appController._stageView
    stageView.switchToFreeCamera()
    return stageView._dataModel.viewSettings.freeCamera

def _SetFreeCamera(appController, gfCam, isZUp):
    stageView = appController._stageView
    stageView._dataModel.viewSettings.freeCamera = \
            FreeCamera.FromGfCamera(gfCam, isZUp)

def _testFreeCameraTumble(appController):
    # test tumbling around
    freeCam = _GetFreeCamera(appController)
    freeCam.Tumble(90, 0)
    appController._takeShot("block_R.png")
    freeCam.Tumble(90, 0)
    appController._takeShot("block_B.png")
    freeCam.Tumble(90, 0)
    appController._takeShot("block_L.png")
    freeCam.rotTheta = 0
    appController._takeShot("block_F.png")

    freeCam.Tumble(0, 45)
    appController._takeShot("phi45.png")

def _testFreeCameraAdjustDistance(appController):
    freeCam = _GetFreeCamera(appController)
    freeCam.AdjustDistance(0.5)
    appController._takeShot("adjustDist_05.png")

    freeCam.AdjustDistance(2)
    appController._takeShot("adjustDist_05_20.png")

def _testFreeCameraTruck(appController):
    freeCam = _GetFreeCamera(appController)
    freeCam.Truck(1, 1)
    appController._takeShot("truck_1_1.png")

    freeCam.Truck(-2, -2)
    appController._takeShot("truck_-1_-1.png")

    # test that tumble and truck does the right thing
    freeCam.rotTheta = -45
    freeCam.Truck(1, 0)
    appController._takeShot("rot_truck.png")

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    start = _GetFreeCamera(appController)
    startGfCam, startIsZUp = Gf.Camera(start._camera), start._isZUp

    appController._takeShot("start.png")

    _testFreeCameraTumble(appController)
    _SetFreeCamera(appController, startGfCam, startIsZUp)

    _testFreeCameraAdjustDistance(appController)
    _SetFreeCamera(appController, startGfCam, startIsZUp)

    _testFreeCameraTruck(appController)
    _SetFreeCamera(appController, startGfCam, startIsZUp)

    appController._takeShot("end.png")
