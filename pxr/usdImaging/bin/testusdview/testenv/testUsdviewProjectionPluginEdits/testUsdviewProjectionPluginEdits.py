#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import UsdShade

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

# Update the connected Projection Plugin
def _updateProjectionPlugin(pluginPaths, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    camera = stage.GetPrimAtPath('/main_cam')
    projectionConnection = camera.GetProperty('outputs:ri:projection')
    projectionConnection.SetConnections(pluginPaths)

def _updateProjectionPluginParam(projectionPluginPath,
                                 attrName, attrValue, appController):
    stage = appController._dataModel.stage
    layer = stage.GetSessionLayer()
    stage.SetEditTarget(layer)

    projectionPlugin = stage.GetPrimAtPath(projectionPluginPath)
    projectionPluginParam = projectionPlugin.GetAttribute(attrName)
    projectionPluginParam.Set(attrValue)

# Test changing the connected ProjectionPlugin.
def testUsdviewInputFunction(appController):
    from os import cpu_count
    print('******CPU COUNT: {}'.format(cpu_count()))
    _modifySettings(appController)

    cylPath = '/main_cam/CylCamera.outputs:result'
    cylOutput = '/main_cam/CylCamera.outputs:result'
    paniniPath = '/main_cam/PaniniCam'
    paniniOutput = '/main_cam/PaniniCam.outputs:result'

    paniniFov = "inputs:ri:fov"

    appController._takeShot("CylinderCamera.png", waitForConvergence=True)
    
    _updateProjectionPlugin([paniniOutput], appController)
    appController._takeShot("PaniniCamera.png", waitForConvergence=True)

    _updateProjectionPluginParam(paniniPath, paniniFov, 40.0, appController)
    appController._takeShot("PaniniCameraModified.png", waitForConvergence=True)

    _updateProjectionPlugin([], appController)
    appController._takeShot("ResetCamera.png", waitForConvergence=True)


