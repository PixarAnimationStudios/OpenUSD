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
from PySide import QtGui
from usdviewContextMenuItem import UsdviewContextMenuItem
import os
import sys

#
# Edit the following to alter the per-node context menu.
#
# Every entry should be an object derived from NodeContextMenuItem,
# defined below.
#
def _GetContextMenuItems(mainWindow, item):
    return [JumpToEnclosingModelItem(mainWindow, item),
            JumpToBoundMaterialMenuItem(mainWindow, item),
            JumpToMasterMenuItem(mainWindow, item),
            SeparatorMenuItem(mainWindow, item),
            ToggleVisibilityMenuItem(mainWindow, item),
            VisOnlyMenuItem(mainWindow, item),
            RemoveVisMenuItem(mainWindow, item),
            SeparatorMenuItem(mainWindow, item),
            LoadOrUnloadMenuItem(mainWindow, item),
            ActiveMenuItem(mainWindow, item),
            SeparatorMenuItem(mainWindow, item),
            CopyPrimPathMenuItem(mainWindow, item),
	    CopyModelPathMenuItem(mainWindow, item),
	    SeparatorMenuItem(mainWindow, item),
	    IsolateAssetMenuItem(mainWindow, item)]

#
# The base class for per-node context menu items.
#
class NodeContextMenuItem(UsdviewContextMenuItem):

    def __init__(self, mainWindow, item):
    	self._currentNodes = mainWindow._currentNodes
    	self._currentFrame = mainWindow._currentFrame
        self._mainWindow = mainWindow
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
class SeparatorMenuItem(NodeContextMenuItem):

    def IsSeparator(self):
        return True

#
# Replace each selected prim with its enclosing model prim, if it has one,
# in the selection set
#
class JumpToEnclosingModelItem(NodeContextMenuItem):

    def IsEnabled(self):
        from common import GetEnclosingModelPrim

        for p in self._currentNodes:
            if GetEnclosingModelPrim(p) is not None:
                return True
        return False

    def GetText(self):
        return "Jump to Enclosing Model"

    def RunCommand(self):
        self._mainWindow.jumpToEnclosingModelSelectedPrims()

#
# Replace each selected prim with the Material it or its closest ancestor is
# bound to. 
#
class JumpToBoundMaterialMenuItem(NodeContextMenuItem):

    def __init__(self, mainWindow, item):
        NodeContextMenuItem.__init__(self, mainWindow, item)
        from common import GetClosestBoundMaterial

        self._material = None
        for p in self._currentNodes:
            material, bound = GetClosestBoundMaterial(p)
            if material is not None:
                self._material = material

    def IsEnabled(self):
        return self._material is not None

    def GetText(self):
        text = "Jump to Bound Material"
        if self._material is not None:
            text += "(%s)" % self._material.GetName()
        return text

    def RunCommand(self):
        self._mainWindow.jumpToBoundMaterialSelectedPrims()

#
# Replace each selected instance prim with its master prim.
#
class JumpToMasterMenuItem(NodeContextMenuItem):

    def IsEnabled(self):
        for p in self._currentNodes:
            if p.IsInstance():
                return True
        return False

    def GetText(self):
        return "Jump to Master"

    def RunCommand(self):
        self._mainWindow.jumpToMasterSelectedPrims()

#
# Allows you to activate/deactivate a prim in the graph
#
class ActiveMenuItem(NodeContextMenuItem):

    def GetText(self):
        if self._currentNodes[0].IsActive():
            return "Deactivate"
        else:
            return "Activate"

    def RunCommand(self):
        active = self._currentNodes[0].IsActive()
        if active:
            self._mainWindow.deactivateSelectedPrims()
        else:
            self._mainWindow.activateSelectedPrims()

#
# Allows you to vis or invis a prim in the graph, based on its current
# resolved visibility
#
class ToggleVisibilityMenuItem(NodeContextMenuItem):

    def __init__(self, mainWindow, item):
        NodeContextMenuItem.__init__(self, mainWindow, item)
        from pxr import UsdGeom
        self._imageable = False
        self._isVisible = False
        for prim in self._currentNodes:
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
            self._mainWindow.invisSelectedPrims()
        else:
            self._mainWindow.visSelectedPrims()


#
# Allows you to vis-only a prim in the graph
#
class VisOnlyMenuItem(NodeContextMenuItem):

    def IsEnabled(self):
        from pxr import UsdGeom
        for prim in self._currentNodes:
            if prim.IsA(UsdGeom.Imageable):
                return True
        return False
    
    def GetText(self):
        return "Vis Only"

    def RunCommand(self):
        self._mainWindow.visOnlySelectedPrims()

#
# Remove any vis/invis authored on selected prims
#
class RemoveVisMenuItem(NodeContextMenuItem):
    
    def IsEnabled(self):
        from common import HasSessionVis
        for prim in self._currentNodes:
            if HasSessionVis(prim):
                return True

        return False

    def GetText(self):
        return "Remove Session Visibility"

    def RunCommand(self):
        self._mainWindow.removeVisSelectedPrims()


#
# Toggle load-state on the selected prims, if loadable
#
class LoadOrUnloadMenuItem(NodeContextMenuItem):

    def __init__(self, mainWindow, item):
        NodeContextMenuItem.__init__(self, mainWindow, item)
        from common import GetPrimsLoadability
        # Use the descendent-pruned selection set to avoid redundant
        # traversal of the stage to answer isLoaded...
        self._loadable, self._loaded = GetPrimsLoadability(self._mainWindow._prunedCurrentNodes)

    def IsEnabled(self):
        return self._loadable
    
    def GetText(self):
        return "Unload" if self._loaded else "Load"

    def RunCommand(self):
        if self._loaded:
            self._mainWindow.unloadSelectedPrims()
        else:
            self._mainWindow.loadSelectedPrims()



#
# Copies the paths of the currently selected prims to the clipboard
#
class CopyPrimPathMenuItem(NodeContextMenuItem):

    def GetText(self):
        if len(self._currentNodes) > 1:
            return "Copy Prim Paths"
        return "Copy Prim Path"

    def RunCommand(self):
        pathlist = [str(p.GetPath()) for p in self._currentNodes]
        pathStrings = '\n'.join(pathlist)

        cb = QtGui.QApplication.clipboard()
        cb.setText(pathStrings, QtGui.QClipboard.Selection )
        cb.setText(pathStrings, QtGui.QClipboard.Clipboard )

#
#  Copies the path of the first-selected prim's enclosing model
#  to the clipboard, if the prim is inside a model
#
class CopyModelPathMenuItem(NodeContextMenuItem):

    def __init__(self, mainWindow, item):
        NodeContextMenuItem.__init__(self, mainWindow, item)
        from common import GetEnclosingModelPrim

        self._modelPrim = GetEnclosingModelPrim(self._currentNodes[0]) if \
            len(self._currentNodes) == 1 else None
    
    def IsEnabled(self):
        return self._modelPrim

    def GetText(self):
        name = ( "(%s)" % self._modelPrim.GetName() ) if self._modelPrim else ""
        return "Copy Enclosing Model %s Path" % name

    def RunCommand(self):
        modelPath = str(self._modelPrim.GetPath())
        cb = QtGui.QApplication.clipboard()
        cb.setText(modelPath, QtGui.QClipboard.Selection )
        cb.setText(modelPath, QtGui.QClipboard.Clipboard )



#
# Copies the current node and subtree to a file of the user's choosing
# XXX This is not used, and does not work. Leaving code in for now for
# future reference/inspiration
#
class IsolateCopyNodeMenuItem(NodeContextMenuItem):

    def GetText(self):
        return "Isolate Copy of Prim..."
	
    def RunCommand(self):
        inFile = self._currentNodes[0].GetScene().GetUsdFile()

        guessOutFile = os.getcwd() + "/" + self._currentNodes[0].GetName() + "_copy.usd"
        (outFile, _) = QtGui.QFileDialog.getSaveFileName(None,
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
                ' -sourcePath ' + self._currentNodes[0].GetPath() + '; ' +
                'usdview ' + outFile + ' &')

        sys.stdout.write( "Done!\n" )

    def IsEnabled(self):
        return len(self._currentNodes) == 1 and self._currentNodes[0].GetActive()

	
#
# Launches usdview on the asset instantiated at the selected prim, as 
# defined by USD assetInfo present on the prim
#
class IsolateAssetMenuItem(NodeContextMenuItem):

    def __init__(self, mainWindow, item):
        NodeContextMenuItem.__init__(self, mainWindow, item)

        self._assetName = None
        if len(self._currentNodes) == 1:
            from pxr import Usd
            model = Usd.ModelAPI(self._currentNodes[0])
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

