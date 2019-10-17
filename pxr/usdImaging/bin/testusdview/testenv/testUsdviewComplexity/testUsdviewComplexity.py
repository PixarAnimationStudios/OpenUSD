#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

# Test with low refinement.
def _testLowRefinement(appController):
    _setComplexity(appController, RefinementComplexities.LOW)
    _takeShot(appController, "low.png")

# Test with medium refinement.
def _testMediumRefinement(appController):
    _setComplexity(appController, RefinementComplexities.MEDIUM)
    _takeShot(appController, "medium.png")

# Test with high refinement.
def _testHighRefinement(appController):
    _setComplexity(appController, RefinementComplexities.HIGH)
    _takeShot(appController, "high.png")

# Test with very high refinement.
def _testVeryHighRefinement(appController):
    _setComplexity(appController, RefinementComplexities.VERY_HIGH)
    _takeShot(appController, "very_high.png")

# Test that the complexity setting works properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testLowRefinement(appController)
    _testMediumRefinement(appController)
    _testHighRefinement(appController)
    _testVeryHighRefinement(appController)
