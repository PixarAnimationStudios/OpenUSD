#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Set SceneMaterials enabled or disabled and refresh the view
def _setSceneMaterials(appController, sceneMaterialsEnabled):
    appController._ui.actionEnable_Scene_Materials.setChecked(sceneMaterialsEnabled)
    appController._toggleEnableSceneMaterials()

# Test that enabling and disabling scene materials works properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)

    # Disable Scene Materials
    _setSceneMaterials(appController, False)
    filename = "disabledSceneMaterials.png"
    appController._takeShot(filename, waitForConvergence=True)

    # Enable Scene Materials
    _setSceneMaterials(appController, True)
    filename = "enabledSceneMaterials.png"
    appController._takeShot(filename, waitForConvergence=True)
