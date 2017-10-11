#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to # it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) #    of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF # ANY
# KIND, either express or implied. See the Apache License for the # specific
# language governing permissions and limitations under the Apache # License.

# This class should be the only object that knows about the details of the
# UsdView mainWindow.  If that API ever changes, we should only need to update
# this class.
class PlugContext(object):
    '''
    This class is an interface that provides access to usdview context for
    usdview plugins.  It abstracts away the implementation of usdview so that
    the core can change without affecting plugins.
    '''

    def __init__(self, mainWindow):
        self._mainWindow = mainWindow

    def GetQMainWindow(self):
        ''' Returns a QWidget object that other widgets can use as a parent. '''

        return self._mainWindow

    def GetUsdStage(self):
        ''' Returns the current Usd stage. '''

        return self._mainWindow._stage

    def GetCurrentGfCamera(self):
        ''' Returns the last computed Gf Camera. '''
        return self._mainWindow._stageView.gfCamera

    def GetCurrentFrame(self):
        ''' Returns the current frame. '''

        return self._mainWindow._currentFrame

    def GetModelsFromSelection(self):
        ''' Returns selected models.  this will walk up to find the nearest model.
        Note, this may return "group"'s if they are selected. '''

        models = []
        items = self._mainWindow.getSelectedItems()
        for item in items:
            currItem = item
            while currItem and not currItem.node.IsModel():
                currItem = currItem.parent()
            if currItem:
                models.append(currItem.node)

        return models

    def GetSelectedPrimsOfType(self, schemaType):
        ''' Returns selected prims of the provided schemaType (TfType).'''
        prims = []
        items = self._mainWindow.getSelectedItems()
        for item in items:
            if item.node.IsA(schemaType):
                prims.append(item.node)

        return prims

    def GetCurrentNodes(self):
        ''' Returns the current nodes. '''

        return self._mainWindow._currentNodes

    def GetSelectedPaths(self):
        ''' Returns the paths for the current selections. '''

        return [item.node.GetPath()
                for item in self._mainWindow.getSelectedItems()]

    def GetConfigDir(self):
        ''' Returns the config dir, typically ~/.usdview/. '''

        return self._mainWindow._outputBaseDirectory()

    def GetInputFilePath(self):
        ''' Returns the file that usdview was called on. '''

        return self._mainWindow._parserData.usdFile

    def PrintStatus(self, msg):
        ''' Prints a status message. '''

        self._mainWindow.statusMessage(msg)

    def GetStageView(self):
        ''' Returns the stageView object. '''

        return self._mainWindow._stageView

    def GetSettings(self):
        ''' Returns settings object. '''

        return self._mainWindow._settings

    # Screen capture functionality.
    def GrabWindowShot(self):
        ''' Returns a QImage of the full usdview window. '''

        return self._mainWindow.GrabWindowShot()

    def GrabViewportShot(self):
        ''' Returns a QImage of the current stage view in usdview. '''

        return self._mainWindow.GrabViewportShot()
