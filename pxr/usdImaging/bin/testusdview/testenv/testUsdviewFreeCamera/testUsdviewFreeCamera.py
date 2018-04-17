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
from pxr.Usdviewq.qt import QtWidgets
from pxr import Gf

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

def _takeShot(appController, fileName):
    appController._stageView.updateGL()
    QtWidgets.QApplication.processEvents()
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

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
    freeCam.rotTheta += 90
    _takeShot(appController, "block_R.png")
    freeCam.rotTheta += 90
    _takeShot(appController, "block_B.png")
    freeCam.rotTheta += 90
    _takeShot(appController, "block_L.png")
    freeCam.rotTheta = 0
    _takeShot(appController, "block_F.png")

    freeCam.rotPhi = 45
    _takeShot(appController, "phi45.png")

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    start = _GetFreeCamera(appController)
    startGfCam, startIsZUp = Gf.Camera(start._camera), start._isZUp

    _takeShot(appController, "start.png")

    _testFreeCameraTumble(appController)
    _SetFreeCamera(appController, startGfCam, startIsZUp)

    _takeShot(appController, "end.png")
