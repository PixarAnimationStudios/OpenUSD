#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function

from .qt import QtCore, QtGui, QtWidgets
from .usdviewContextMenuItem import UsdviewContextMenuItem
import os, subprocess, sys
from pxr import Ar
from pxr.UsdUtils.toolPaths import FindUsdBinary

#
# Specialized context menu for running commands in the layer stack view.
#
class LayerStackContextMenu(QtWidgets.QMenu):

    def __init__(self, parent, item):
        QtWidgets.QMenu.__init__(self, parent)
        self._menuItems = _GetContextMenuItems(item)

        for menuItem in self._menuItems:
            if menuItem.isValid():
                # create menu actions
                action = self.addAction(menuItem.GetText(), menuItem.RunCommand)

                # set enabled
                if not menuItem.IsEnabled():
                    action.setEnabled(False)


def _GetContextMenuItems(item):
    return [OpenLayerMenuItem(item),
            UsdviewLayerMenuItem(item),
            CopyLayerPathMenuItem(item),
            CopyLayerIdentifierMenuItem(item),
            CopyPathMenuItem(item),
            MuteOrUnmuteLayerMenuItem(item)]

#
# The base class for layer stack context menu items.
#
class LayerStackContextMenuItem(UsdviewContextMenuItem):

    def __init__(self, item):
        self._item = item

    def IsEnabled(self):
        return True

    def GetText(self):
        return ""

    def RunCommand(self):
        pass

#
# Opens the layer using usdedit.
#
class OpenLayerMenuItem(LayerStackContextMenuItem):
    def GetText(self):
        from .common import PrettyFormatSize
        fileSize = 0
        if (hasattr(self._item, "layerPath")
           and os.path.isfile(getattr(self._item, "layerPath"))):
            fileSize = os.path.getsize(getattr(self._item, "layerPath"))
        if fileSize:
            return "Open Layer In Editor (%s)" % PrettyFormatSize(fileSize)
        else:
            return "Open Layer In Editor"

    def IsEnabled(self):
        return hasattr(self._item, "layerPath")

    def RunCommand(self):
        if not self._item:
            return

        # Get layer path from item
        layerPath = getattr(self._item, "layerPath")
        if not layerPath:
            print("Error: Could not find layer file.")
            return

        if Ar.IsPackageRelativePath(layerPath):
            layerName = os.path.basename(
                Ar.SplitPackageRelativePathInner(layerPath)[1])
        else:
            layerName = os.path.basename(layerPath)
        
        layerName += ".tmp"

        usdeditExe = FindUsdBinary('usdedit')
        if not usdeditExe:
            print("Warning: Could not find 'usdedit', expected it to be in PATH.")
            return

        print("Opening file: %s" % layerPath)

        command =  [usdeditExe,'-n',layerPath,'-p',layerName]

        subprocess.Popen(command, close_fds=True)

#
# Opens the layer using usdview.
#
class UsdviewLayerMenuItem(LayerStackContextMenuItem):

    def GetText(self):
        return "Open Layer In usdview"

    def IsEnabled(self):
        return hasattr(self._item, "layerPath")

    def RunCommand(self):
        if not self._item:
            return

        # Get layer path from item
        layerPath = getattr(self._item, "layerPath")
        if not layerPath:
            return

        print("Spawning usdview %s" % layerPath)
        os.system("usdview %s &" % layerPath)

#
# Copy the layer path to clipboard
#
class CopyLayerPathMenuItem(LayerStackContextMenuItem):
    def GetText(self):
        return "Copy Layer Path"

    def RunCommand(self):
        if not self._item:
            return

        layerPath = getattr(self._item, "layerPath")
        if not layerPath:
            return

        cb = QtWidgets.QApplication.clipboard()
        cb.setText(layerPath, QtGui.QClipboard.Selection )
        cb.setText(layerPath, QtGui.QClipboard.Clipboard )

#
# Copy the layer identifier to clipboard
#
class CopyLayerIdentifierMenuItem(LayerStackContextMenuItem):
    def GetText(self):
        return "Copy Layer Identifier"

    def RunCommand(self):
        if not self._item:
            return

        identifier = getattr(self._item, "identifier")
        if not identifier:
            return

        cb = QtWidgets.QApplication.clipboard()
        cb.setText(identifier, QtGui.QClipboard.Selection )
        cb.setText(identifier, QtGui.QClipboard.Clipboard )

#
# Copy the prim path to clipboard, if there is one
#
class CopyPathMenuItem(LayerStackContextMenuItem):
    def GetText(self):
        return "Copy Object Path"

    def RunCommand(self):
        if not self._item:
            return

        path = getattr(self._item, "path")
        if not path:
            return

        path = str(path)
        cb = QtWidgets.QApplication.clipboard()
        cb.setText(path, QtGui.QClipboard.Selection )
        cb.setText(path, QtGui.QClipboard.Clipboard )

    def IsEnabled(self):
        return hasattr(self._item, "path")

#
# Mute/Unmute the layer represented by this menu item
#
class MuteOrUnmuteLayerMenuItem(LayerStackContextMenuItem):
    def GetText(self):
        stage = getattr(self._item, "stage")
        identifier = getattr(self._item, "identifier")
        return 'Unmute Layer' \
                 if stage.IsLayerMuted(identifier) \
                    else 'Mute Layer'

    def RunCommand(self):
        if not self._item:
            return

        identifier = getattr(self._item, "identifier")
        if not identifier:
            return

        stage = getattr(self._item, "stage")
        
        if not stage.IsLayerMuted(identifier):
            stage.MuteLayer(identifier)
        else:
            stage.UnmuteLayer(identifier)
        
    def IsEnabled(self):
        return True
