#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

def testUsdviewInputFunction(appController):

    _modifySettings(appController)
    # We are using a progressive renderer, so wait for images to converge
    appController._takeShot("viewport.png", waitForConvergence=True)
