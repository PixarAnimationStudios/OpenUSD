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
            SelectBoundPreviewMaterialMenuItem(appController, item),
            SelectBoundFullMaterialMenuItem(appController, item),
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
        self._selectionDataModel = appController._dataModel.selection
        self._currentFrame = appController._dataModel.currentFrame
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

        for p in self._selectionDataModel.getPrims():
            if GetEnclosingModelPrim(p) is not None:
                return True
        return False

    def GetText(self):
        return "Jump to Enclosing Model"

    def RunCommand(self):
        self._appController.selectEnclosingModel()

#
# Replace each selected prim with the "preview" Material it is bound to.
#
class SelectBoundPreviewMaterialMenuItem(PrimContextMenuItem):

    def __init__(self, appController, item):
        PrimContextMenuItem.__init__(self, appController, item)
        from pxr import UsdShade 

        self._boundPreviewMaterial = None
        self._bindingRel = None
        for p in self._selectionDataModel.getPrims():
            (self._boundPreviewMaterial, self._bindingRel) = \
                UsdShade.MaterialBindingAPI(p).ComputeBoundMaterial(
                    UsdShade.Tokens.preview)
            if self._boundPreviewMaterial:
                break

    def IsEnabled(self):
        return bool(self._boundPreviewMaterial)

    def GetText(self):
        if self._boundPreviewMaterial:
            isPreviewBindingRel = 'preview' in self._bindingRel.SplitName()
            return "Select Bound Preview Material (%s%s)" % (
            self._boundPreviewMaterial.GetPrim().GetName(),
            "" if isPreviewBindingRel else " from generic binding")
        else:
            return "Select Bound Preview Material (None)"

    def RunCommand(self):
        self._appController.selectBoundPreviewMaterial()

#
# Replace each selected prim with the "preview" Material it is bound to.
#
class SelectBoundFullMaterialMenuItem(PrimContextMenuItem):

    def __init__(self, appController, item):
        PrimContextMenuItem.__init__(self, appController, item)
        from pxr import UsdShade 

        self._boundFullMaterial = None
        self._bindingRel = None
        for p in self._selectionDataModel.getPrims():
            (self._boundFullMaterial, self._bindingRel) = \
                UsdShade.MaterialBindingAPI(p).ComputeBoundMaterial(
                    UsdShade.Tokens.full)
            if self._boundFullMaterial:
                break

    def IsEnabled(self):
        return bool(self._boundFullMaterial)

    def GetText(self):
        if self._boundFullMaterial:
            isFullBindingRel = 'full' in self._bindingRel.SplitName()
            return "Select Bound Full Material (%s%s)" % (
            self._boundFullMaterial.GetPrim().GetName(),
            "" if isFullBindingRel else " from generic binding")
        else:
            return "Select Bound Full Material (None)"

    def RunCommand(self):
        self._appController.selectBoundFullMaterial()


#
# Allows you to activate/deactivate a prim in the graph
#
class ActiveMenuItem(PrimContextMenuItem):

    def GetText(self):
        if self._selectionDataModel.getFocusPrim().IsActive():
            return "Deactivate"
        else:
            return "Activate"

    def RunCommand(self):
        active = self._selectionDataModel.getFocusPrim().IsActive()
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
        for prim in self._selectionDataModel.getPrims():
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
        for prim in self._selectionDataModel.getPrims():
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
        for prim in self._selectionDataModel.getPrims():
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
        self._loadable, self._loaded = GetPrimsLoadability(
            self._selectionDataModel.getLCDPrims())

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
        if len(self._selectionDataModel.getPrims()) > 1:
            return "Copy Prim Paths"
        return "Copy Prim Path"

    def RunCommand(self):
        pathlist = [str(p.GetPath())
            for p in self._selectionDataModel.getPrims()]
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

        if len(self._selectionDataModel.getPrims()) == 1:
            self._modelPrim = GetEnclosingModelPrim(
                self._selectionDataModel.getFocusPrim())
        else:
            self._modelPrim = None

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
        focusPrim = self._selectionDataModel.getFocusPrim()

        inFile = focusPrim.GetScene().GetUsdFile()

        guessOutFile = os.getcwd() + "/" + focusPrim.GetName() + "_copy.usd"
        (outFile, _) = QtWidgets.QFileDialog.getSaveFileName(None,
            "Specify the Usd file to create", guessOutFile, 'Usd files (*.usd)')
        if (outFile.rsplit('.')[-1] != 'usd'):
            outFile += '.usd'

        if inFile == outFile:
            sys.stderr.write( "Cannot isolate a copy to the source usd!\n" )
            return

        sys.stdout.write( "Writing copy to new file '%s' ... " % outFile )
        sys.stdout.flush()

        os.system( 'usdcopy -inUsd ' + inFile +
                ' -outUsd ' + outFile + ' ' +
                ' -sourcePath ' + focusPrim.GetPath() + '; ' +
                'usdview ' + outFile + ' &')

        sys.stdout.write( "Done!\n" )

    def IsEnabled(self):
        numSelectedPrims = len(self._selectionDataModel.getPrims())
        focusPrimActive = self._selectionDataModel.getFocusPrim().GetActive()

        return numSelectedPrims == 1 and focusPrimActive


#
# Launches usdview on the asset instantiated at the selected prim, as
# defined by USD assetInfo present on the prim
#
class IsolateAssetMenuItem(PrimContextMenuItem):

    def __init__(self, appController, item):
        PrimContextMenuItem.__init__(self, appController, item)

        self._assetName = None
        if len(self._selectionDataModel.getPrims()) == 1:
            from pxr import Usd
            model = Usd.ModelAPI(self._selectionDataModel.getFocusPrim())
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

