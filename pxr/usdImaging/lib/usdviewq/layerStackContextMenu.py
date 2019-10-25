#
# Copyright 2016 Pixar
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
from qt import QtCore, QtGui, QtWidgets
from usdviewContextMenuItem import UsdviewContextMenuItem
import os, subprocess
from pxr import Ar

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
            CopyPathMenuItem(item)]

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
    # XXX: Note that this logic is duplicated from usddiff
    # see bug 150247 for centralizing this API.
    def _FindUsdEdit(self):
        import platform
        from distutils.spawn import find_executable
        usdedit = find_executable('usdedit')
        if not usdedit:
            usdedit = find_executable('usdedit', path=os.path.abspath(os.path.dirname(sys.argv[0])))

        if not usdedit and (platform.system() == 'Windows'):
            for path in os.environ['PATH'].split(os.pathsep):
                base = os.path.join(path, 'usdedit')
                for ext in ['.cmd', '']:
                    if os.access(base + ext, os.X_OK):
                        usdedit = base + ext

        return usdedit

    def GetText(self):
        from common import PrettyFormatSize
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
            print "Error: Could not find layer file."
            return

        if Ar.IsPackageRelativePath(layerPath):
            layerName = os.path.basename(
                Ar.SplitPackageRelativePathInner(layerPath)[1])
        else:
            layerName = os.path.basename(layerPath)
        
        layerName += ".tmp"

        usdeditExe = self._FindUsdEdit()
        if not usdeditExe:
            print "Warning: Could not find 'usdedit', expected it to be in PATH."
            return

        print "Opening file: %s" % layerPath

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

        print "Spawning usdview %s" % layerPath
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
