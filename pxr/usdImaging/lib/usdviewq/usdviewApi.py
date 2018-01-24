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

import types

from pxr import Gf


class UsdviewApi(object):
    """This class is an interface that provides access to Usdview context for
    Usdview plugins and other clients. It abstracts away the implementation of
    Usdview so that the core can change without affecting clients.
    """

    def __init__(self, appController):
        self.__appController = appController

    @property
    def dataModel(self):
        """Usdview's active data model object."""

        return self.__appController._dataModel

    @property
    def stage(self):
        """The current Usd.Stage."""

        return self.__appController._dataModel.stage

    @property
    def frame(self):
        """The current frame."""

        return self.__appController._dataModel.currentFrame

    @property
    def prim(self):
        """The focus prim from the prim selection."""

        return self.__appController._dataModel.selection.getFocusPrim()

    @property
    def selectedPrims(self):
        """A list of all currently selected prims."""

        return self.__appController._dataModel.selection.getPrims()

    @property
    def selectedPaths(self):
        """A list of the paths of all currently selected prims."""

        return [item.prim.GetPath() for item in
            self.__appController._dataModel.selection.getPrimPaths()]

    @property
    def selectedInstances(self):
        """The current prim instance selection. This is a dictionary where each
        key is a prim and each value is a set of instance ids selected from that
        prim.
        """

        return self.__appController._dataModel.selection.getPrimInstances()

    @property
    def spec(self):
        """The currently selected Sdf.Spec from the Composition tab."""

        return self.__appController._currentSpec

    @property
    def layer(self):
        """The currently selected Sdf.Layer in the Composition tab."""

        return self.__appController._currentLayer

    @property
    def currentGfCamera(self):
        """A copy of the last computed Gf Camera."""

        if self.__appController._stageView:
            return Gf.Camera(self.__appController._stageView.gfCamera)
        else:
            return None

    # This needs to be the last property added because otherwise the @property
    # decorator will call this method rather than the actual property decorator.
    @property
    def property(self):
        """The focus property from the property selection."""

        return self.__appController._dataModel.selection.getFocusProp()

    def GetQMainWindow(self):
        """Returns a QWidget object that other widgets can use as a parent."""

        return self.__appController._mainWindow

    def GetModelsFromSelection(self):
        """Returns selected models.  this will walk up to find the nearest model.
        Note, this may return "group"'s if they are selected.
        """

        models = []
        items = self.__appController.getSelectedItems()
        for item in items:
            currItem = item
            while currItem and not currItem.prim.IsModel():
                currItem = currItem.parent()
            if currItem:
                models.append(currItem.prim)

        return models

    def GetSelectedPrimsOfType(self, schemaType):
        """Returns selected prims of the provided schemaType (TfType)."""

        prims = []
        items = self.__appController.getSelectedItems()
        for item in items:
            if item.prim.IsA(schemaType):
                prims.append(item.prim)

        return prims

    def GetConfigDir(self):
        """Returns the config dir, typically ~/.usdview/."""

        return self.__appController._outputBaseDirectory()

    def GetInputFilePath(self):
        """Returns the file that usdview was called on."""

        return self.__appController._parserData.usdFile

    def PrintStatus(self, msg):
        """Prints a status message."""

        self.__appController.statusMessage(msg)

    def GetStageView(self):
        """Returns the stageView object."""

        return self.__appController._stageView

    def GetSettings(self):
        """Returns settings object."""

        return self.__appController._settings

    # Screen capture functionality.
    def GrabWindowShot(self):
        """Returns a QImage of the full usdview window."""

        return self.__appController.GrabWindowShot()

    def GrabViewportShot(self):
        """Returns a QImage of the current stage view in usdview."""

        return self.__appController.GrabViewportShot()
