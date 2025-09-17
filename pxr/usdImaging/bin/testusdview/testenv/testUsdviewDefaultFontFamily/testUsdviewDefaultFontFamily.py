#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Usdviewq.qt import QtWidgets
from pxr.Usdviewq.common import DefaultFontFamily

def _testDefaultFontFamily(appController):
    assert appController._mainWindow.font().family() == \
            DefaultFontFamily.FONT_FAMILY
    assert appController._stageView._hud._HUDFont.family() == \
            DefaultFontFamily.MONOSPACE_FONT_FAMILY

def testUsdviewInputFunction(appController):
    _testDefaultFontFamily(appController)
