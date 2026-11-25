#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Vt

testMeshPath = '/World/TestMesh'

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

# Author an attribute value on the session layer.
def _updateMeshAttr(meshPath, attrName, attrValue, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)
    mesh = stage.GetPrimAtPath(meshPath)
    mesh.GetAttribute(attrName).Set(attrValue)

def _setCreaseSharpnesses(perCrease, perCreaseEdge, appController):
    _updateMeshAttr(testMeshPath, 'creaseSharpnesses',
                    Vt.FloatArray(perCrease), appController)

def _setHoleIndices(holeIndices, appController):
    _updateMeshAttr(testMeshPath, 'holeIndices',
                    Vt.IntArray(holeIndices), appController)

# Test editing subdivision tags (crease sharpness and holes).
def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    appController._takeShot("0_initial.png", waitForConvergence=True)

    # Soften all creases.
    _setCreaseSharpnesses([2, 2], [2, 2, 2, 2], appController)
    appController._takeShot("1_softCreases.png", waitForConvergence=True)

    # Remove crease sharpness entirely.
    _setCreaseSharpnesses([0, 0], [0, 0, 0, 0], appController)
    appController._takeShot("2_noCreases.png", waitForConvergence=True)

    # Vary sharpness per crease and per crease edge.
    _setCreaseSharpnesses([10, 0], [10, 0, 0, 10], appController)
    appController._takeShot("3_mixedCreases.png", waitForConvergence=True)

    # Punch a hole in the center face.
    _setHoleIndices([4], appController)
    appController._takeShot("4_centerHole.png", waitForConvergence=True)

    # Move the holes to the corner faces.
    _setHoleIndices([0, 8], appController)
    appController._takeShot("5_cornerHoles.png", waitForConvergence=True)

    # Clear the holes.
    _setHoleIndices([], appController)
    appController._takeShot("6_noHoles.png", waitForConvergence=True)
