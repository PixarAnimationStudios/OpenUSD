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
from qt import QtGui, QtWidgets
from usdviewContextMenuItem import UsdviewContextMenuItem
import os
import sys

#
# Edit the following to alter the per-prim context menu.
#
# Every entry should be an object derived from PrimContextMenuItem,
# defined below.
#
def _GetContextMenuItems(appController, item):
    return [JumpToEnclosingModelItem(appController, item),
            JumpToBoundMaterialMenuItem(appController, item),
            SeparatorMenuItem(appController, item),
            ToggleVisibilityMenuItem(appController, item),
            VisOnlyMenuItem(appController, item),
            RemoveVisMenuItem(appController, item),
            SeparatorMenuItem(appController, item),
            LoadOrUnloadMenuItem(appController, item),
            ActiveMenuItem(appController, item),
            SeparatorMenuItem(appController, item),
            CopyPrimPathMenuItem(appController, item),
            CopyModelPathMenuItem(appController, item),
            SeparatorMenuItem(appController, item),
            IsolateAssetMenuItem(appController, item)]

#
# The base class for per-prim context menu items.
#
class PrimContextMenuItem(UsdviewContextMenuItem):

    def __init__(self, appController, item):
        self._currentPrims = appController._currentPrims
        self._currentFrame = appController._rootDataModel.currentFrame
        self._appController = appController
        self._item = item

    def IsVisible(self):
        return True

    def IsEnabled(self):
        return True

    def IsSeparator(self):
        return False

    def GetText(self):
        return ""

    def RunCommand(self):
        return True

#
# Puts a separator in the context menu
#
class SeparatorMenuItem(PrimContextMenuItem):

    def IsSeparator(self):
        return True

#
# Replace each selected prim with its enclosing model prim, if it has one,
# in the selection set
#
class JumpToEnclosingModelItem(PrimContextMenuItem):

    def IsEnabled(self):
        from common import GetEnclosingModelPrim

        for p in self._currentPrims:
            if GetEnclosingModelPrim(p) is not None:
                return True
        return False

    def GetText(self):
        return "Jump to Enclosing Model"

    def RunCommand(self):
        self._appController.jumpToEnclosingModelSelectedPrims()

#
# Replace each selected prim with the Material it or its closest ancestor is
# bound to.
#
class JumpToBoundMaterialMenuItem(PrimContextMenuItem):

    def __init__(self, appController, item):
        PrimContextMenuItem.__init__(self, appController, item)
        from common import GetClosestBoundMaterial

        self._material = None
        for p in self._currentPrims:
            material, bound = GetClosestBoundMaterial(p)
            if material is not None:
                self._material = material
                break

    def IsEnabled(self):
        return self._material is not None

    def GetText(self):
        return "Jump to Bound Material (%s)" % (self._material.GetName() if
                                                self._material else
                                                "no material bound")
    def RunCommand(self):
        self._appController.jumpToBoundMaterialSelectedPrims()


#
# Allows you to activate/deactivate a prim in the graph
#
class ActiveMenuItem(PrimContextMenuItem):

    def GetText(self):
        if self._currentPrims[0].IsActive():
            return "Deactivate"
        else:
            return "Activate"

    def RunCommand(self):
        active = self._currentPrims[0].IsActive()
        if active:
            self._appController.deactivateSelectedPrims()
        else:
            self._appController.activateSelectedPrims()

#
# Allows you to vis or invis a prim in the graph, based on its current
# resolved visibility
#
class ToggleVisibilityMenuItem(PrimContextMenuItem):

    def __init__(self, appController, item):
        PrimContextMenuItem.__init__(self, appController, item)
        from pxr import UsdGeom
        self._imageable = False
        self._isVisible = False
        for prim in self._currentPrims:
            imgbl = UsdGeom.Imageable(prim)
            if imgbl:
                self._imageable = True
                self._isVisible = (imgbl.ComputeVisibility(self._currentFrame)
                                   == UsdGeom.Tokens.inherited)
                break

    def IsEnabled(self):
        return self._imageable

    def GetText(self):
        return "Make Invisible" if self._isVisible else "Make Visible"

    def RunCommand(self):
        if self._isVisible:
            self._appController.invisSelectedPrims()
        else:
            self._appController.visSelectedPrims()


#
# Allows you to vis-only a prim in the graph
#
class VisOnlyMenuItem(PrimContextMenuItem):

    def IsEnabled(self):
        from pxr import UsdGeom
        for prim in self._currentPrims:
            if prim.IsA(UsdGeom.Imageable):
                return True
        return False

    def GetText(self):
        return "Vis Only"

    def RunCommand(self):
        self._appController.visOnlySelectedPrims()

#
# Remove any vis/invis authored on selected prims
#
class RemoveVisMenuItem(PrimContextMenuItem):

    def IsEnabled(self):
        from common import HasSessionVis
        for prim in self._currentPrims:
            if HasSessionVis(prim):
                return True

        return False

    def GetText(self):
        return "Remove Session Visibility"

    def RunCommand(self):
        self._appController.removeVisSelectedPrims()


#
# Toggle load-state on the selected prims, if loadable
#
class LoadOrUnloadMenuItem(PrimContextMenuItem):

    def __init__(self, appController, item):
        PrimContextMenuItem.__init__(self, appController, item)
        from common import GetPrimsLoadability
        # Use the descendent-pruned selection set to avoid redundant
        # traversal of the stage to answer isLoaded...
        self._loadable, self._loaded = GetPrimsLoadability(self._appController._prunedCurrentPrims)

    def IsEnabled(self):
        return self._loadable

    def GetText(self):
        return "Unload" if self._loaded else "Load"

    def RunCommand(self):
        if self._loaded:
            self._appController.unloadSelectedPrims()
        else:
            self._appController.loadSelectedPrims()



#
# Copies the paths of the currently selected prims to the clipboard
#
class CopyPrimPathMenuItem(PrimContextMenuItem):

    def GetText(self):
        if len(self._currentPrims) > 1:
            return "Copy Prim Paths"
        return "Copy Prim Path"

    def RunCommand(self):
        pathlist = [str(p.GetPath()) for p in self._currentPrims]
        pathStrings = '\n'.join(pathlist)

        cb = QtWidgets.QApplication.clipboard()
        cb.setText(pathStrings, QtGui.QClipboard.Selection )
        cb.setText(pathStrings, QtGui.QClipboard.Clipboard )

#
#  Copies the path of the first-selected prim's enclosing model
#  to the clipboard, if the prim is inside a model
#
class CopyModelPathMenuItem(PrimContextMenuItem):

    def __init__(self, appController, item):
        PrimContextMenuItem.__init__(self, appController, item)
        from common import GetEnclosingModelPrim

        self._modelPrim = GetEnclosingModelPrim(self._currentPrims[0]) if \
            len(self._currentPrims) == 1 else None

    def IsEnabled(self):
        return self._modelPrim

    def GetText(self):
        name = ( "(%s)" % self._modelPrim.GetName() ) if self._modelPrim else ""
        return "Copy Enclosing Model %s Path" % name

    def RunCommand(self):
        modelPath = str(self._modelPrim.GetPath())
        cb = QtWidgets.QApplication.clipboard()
        cb.setText(modelPath, QtGui.QClipboard.Selection )
        cb.setText(modelPath, QtGui.QClipboard.Clipboard )



#
# Copies the current prim and subtree to a file of the user's choosing
# XXX This is not used, and does not work. Leaving code in for now for
# future reference/inspiration
#
class IsolateCopyPrimMenuItem(PrimContextMenuItem):

    def GetText(self):
        return "Isolate Copy of Prim..."

    def RunCommand(self):
        inFile = self._currentPrims[0].GetScene().GetUsdFile()

        guessOutFile = os.getcwd() + "/" + self._currentPrims[0].GetName() + "_copy.usd"
        (outFile, _) = QtWidgets.QFileDialog.getSaveFileName(None,
                                                         "Specify the Usd file to create",
                                                         guessOutFile,
                                                         'Usd files (*.usd)')
        if (outFile.rsplit('.')[-1] != 'usd'):
            outFile += '.usd'

        if inFile == outFile:
            sys.stderr.write( "Cannot isolate a copy to the source usd!\n" )
            return

        sys.stdout.write( "Writing copy to new file '%s' ... " % outFile )
        sys.stdout.flush()

        os.system( 'usdcopy -inUsd ' + inFile +
                ' -outUsd ' + outFile + ' ' +
                ' -sourcePath ' + self._currentPrims[0].GetPath() + '; ' +
                'usdview ' + outFile + ' &')

        sys.stdout.write( "Done!\n" )

    def IsEnabled(self):
        return len(self._currentPrims) == 1 and self._currentPrims[0].GetActive()


#
# Launches usdview on the asset instantiated at the selected prim, as
# defined by USD assetInfo present on the prim
#
class IsolateAssetMenuItem(PrimContextMenuItem):

    def __init__(self, appController, item):
        PrimContextMenuItem.__init__(self, appController, item)

        self._assetName = None
        if len(self._currentPrims) == 1:
            from pxr import Usd
            model = Usd.ModelAPI(self._currentPrims[0])
            name = model.GetAssetName()
            identifier = model.GetAssetIdentifier()
            if name and identifier:
                # Ar API is still settling out...
                # from pxr import Ar
                # identifier = Ar.GetResolver().Resolve("", identifier.path)
                from pxr import Sdf
                layer = Sdf.Layer.Find(identifier.path)
                if layer:
                    self._assetName = name
                    self._filePath = layer.realPath

    def IsEnabled(self):
        return self._assetName

    def GetText(self):
        name = ( " '%s'" % self._assetName ) if self._assetName else ""
        return "usdview asset%s" % name

    def RunCommand(self):
        print "Spawning usdview %s" % self._filePath
        os.system("usdview %s &" % self._filePath)

