#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.UsdAppUtils.complexityArgs import RefinementComplexities
from pxr.Usdviewq.common import RenderModes


# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

    # Make image differences clear.
    appController._dataModel.viewSettings.renderMode = RenderModes.WIREFRAME

# Set the complexity and refresh the view.
def _setComplexity(appController, complexity):
    appController._dataModel.viewSettings.complexity = complexity
    appController._stageView.updateGL()

# Test with low refinement.
def _testLowRefinement(appController):
    _setComplexity(appController, RefinementComplexities.LOW)
    appController._takeShot("low.png")

# Test with medium refinement.
def _testMediumRefinement(appController):
    _setComplexity(appController, RefinementComplexities.MEDIUM)
    appController._takeShot("medium.png")

# Test with high refinement.
def _testHighRefinement(appController):
    _setComplexity(appController, RefinementComplexities.HIGH)
    appController._takeShot("high.png")

# Test with very high refinement.
def _testVeryHighRefinement(appController):
    _setComplexity(appController, RefinementComplexities.VERY_HIGH)
    appController._takeShot("very_high.png")

# Test that the complexity setting works properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testLowRefinement(appController)
    _testMediumRefinement(appController)
    _testHighRefinement(appController)
    _testVeryHighRefinement(appController)
