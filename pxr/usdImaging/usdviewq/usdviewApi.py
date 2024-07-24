#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import types

from pxr import Gf
from .qt import QtCore


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
    def selectedPoint(self):
        """The currently selected world space point."""

        return self.__appController._dataModel.selection.getPoint()

    @property
    def selectedPrims(self):
        """A list of all currently selected prims."""

        return self.__appController._dataModel.selection.getPrims()

    @property
    def selectedPaths(self):
        """A list of the paths of all currently selected prims."""

        return self.__appController._dataModel.selection.getPrimPaths()

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
    def cameraPrim(self):
        """The current camera prim."""

        return self.__appController._dataModel.viewSettings.cameraPrim

    @property
    def currentGfCamera(self):
        """A copy of the last computed Gf Camera."""

        if self.__appController._stageView:
            return Gf.Camera(self.__appController._stageView.gfCamera)
        else:
            return None

    @property
    def viewportSize(self):
        """The width and height of the viewport in pixels."""

        stageView = self.__appController._stageView
        if stageView is not None:
            return stageView.width(), stageView.height()
        else:
            return 0, 0

    @property
    def configDir(self):
        """The config dir, typically ~/.usdview/."""

        return self.__appController._outputBaseDirectory()

    @property
    def stageIdentifier(self):
        """The identifier of the open Usd.Stage's root layer."""

        return self.__appController._dataModel.stage.GetRootLayer().identifier

    @property
    def qMainWindow(self):
        """A QWidget object that other widgets can use as a parent."""

        return self.__appController._mainWindow

    @property
    def viewerMode(self):
        """Whether the app is in viewer mode, with the additional UI around the
        stage view collapsed."""
        return self.__appController.isViewerMode()

    @viewerMode.setter
    def viewerMode(self, value):
        self.__appController.setViewerMode(value)

    # This needs to be the last property added because otherwise the @property
    # decorator will call this method rather than the actual property decorator.
    @property
    def property(self):
        """The focus property from the property selection."""

        return self.__appController._dataModel.selection.getFocusProp()

    def ComputeModelsFromSelection(self):
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

    def ComputeSelectedPrimsOfType(self, schemaType):
        """Returns selected prims of the provided schemaType (TfType)."""

        prims = []
        items = self.__appController.getSelectedItems()
        for item in items:
            if item.prim.IsA(schemaType):
                prims.append(item.prim)

        return prims

    def UpdateGUI(self):
        """Updates the main UI views"""
        self.__appController.updateGUI()

    def PrintStatus(self, msg):
        """Prints a status message."""

        self.__appController.statusMessage(msg)

    def GetSettings(self):
        """Returns the settings object."""

        return self.__appController._configManager.settings

    def ClearPrimSelection(self):
        self.__appController._dataModel.selection.clearPrims()

    def AddPrimToSelection(self, prim):
        self.__appController._dataModel.selection.addPrim(prim)

    # Screen capture functionality.
    def GrabWindowShot(self):
        """Returns a QImage of the full usdview window."""

        return self.__appController.GrabWindowShot()

    def GrabViewportShot(self):
        """Returns a QImage of the current stage view in usdview."""

        return self.__appController.GrabViewportShot()

    def UpdateViewport(self):
        """Schedules a redraw."""
        stageView = self.__appController._stageView
        if stageView is not None:
            stageView.updateGL()

    def SetViewportRenderer(self, plugId):
        """Sets the renderer based on the given ID string.

        The string should be one of the items in GetViewportRendererNames().
        """
        self.__appController._rendererPluginChanged(plugId)

    def GetViewportRendererNames(self):
        """Returns the list of available renderer plugins that can be passed to
        SetViewportRenderer().
        """
        stageView = self.__appController._stageView
        return stageView.GetRendererPlugins() if stageView else []

    def GetViewportCurrentRendererId(self):
        stageView = self.__appController._stageView
        if stageView:
            return stageView.GetCurrentRendererId()
        return None

    def _ExportSession(self, stagePath, defcamName='usdviewCam', imgWidth=None,
            imgHeight=None):
        """Export the free camera (if currently active) and session layer to a
        USD file at the specified stagePath that references the current-viewed
        stage.
        """

        stageView = self.__appController._stageView
        if stageView is not None:
            stageView.ExportSession(stagePath, defcamName='usdviewCam',
                imgWidth=None, imgHeight=None)
