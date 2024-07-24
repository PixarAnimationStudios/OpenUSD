#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Remove any unwanted visuals from the view, and enable autoClip
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.autoComputeClippingPlanes = True

def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    stage = appController._dataModel.stage
    appController._takeShot('start.png')

    stage.SetMetadataByDictKey('expressionVariables', 'TEXTURE_NAME', 'grid_1')
    appController._takeShot('step_1.png')

    stage.SetMetadataByDictKey('expressionVariables', 'TEXTURE_NAME', 'grid_2')
    appController._takeShot('step_2.png')
