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
from PySide import QtGui, QtCore
from pxr import Usd, UsdGeom
from ._usdviewq import Utils

from common import (HasArcsColor, NormalColor, InstanceColor, MasterColor, 
                    AbstractPrimFont, DefinedPrimFont, OverPrimFont, BoldFont)

HALF_DARKER = 150
# Pulled out as a wrapper to facilitate cprofile tracking
def _GetPrimInfo(prim, time):
    return Utils.GetPrimInfo(prim, time)

# This class extends QTreeWidgetItem to also contain all the stage 
# node data associated with it and populate itself with that data.

class NodeViewItem(QtGui.QTreeWidgetItem):
    def __init__(self, node, mainWindow, nodeHasChildren):
        # Do *not* pass a parent.  The client must build the hierarchy.
        # This can dramatically improve performance when building a
        # large hierarchy.
        super(NodeViewItem, self).__init__()

        self.node = node
        self._mainWindow = mainWindow
        self._needsPull = True
        self._needsPush = True
        self._needsChildrenPopulated = nodeHasChildren

        # Initialize these now so loadVis(), _visData() and onClick() can
        # use them without worrying if _pull() has been called.
        self.imageable = False
        self.active = False

        # If we know we'll have children show a norgie, otherwise don't.
        if nodeHasChildren:
            self.setChildIndicatorPolicy(QtGui.QTreeWidgetItem.ShowIndicator)
        else:
            self.setChildIndicatorPolicy(QtGui.QTreeWidgetItem.DontShowIndicator)

    def push(self):
        """Pushes prim data to the UI."""
        # Push to UI.
        if self._needsPush:
            self._needsPush = False
            self._pull()
            self.emitDataChanged()

    def _pull(self):
        """Extracts and stores prim data."""
        if self._needsPull:
            # Only do this once.
            self._needsPull = False

            # Visibility is recursive so the parent must pull before us.
            parent = self.parent()
            if isinstance(parent, NodeViewItem):
                parent._pull()

            # Get our prim info.
            # To avoid Python overhead, request data in batch from C++.
            info = _GetPrimInfo(self.node, self._mainWindow._currentFrame)
            self._extractInfo(info)

    def _extractInfo(self, info):
        ( self.hasArcs,
          self.active,
          self.imageable,
          self.defined,
          self.abstract,
          self.isInMaster,
          self.isInstance,
          isVisibilityInherited,
          self.visVaries,
          self.name,
          self.typeName ) = info

        parent = self.parent()
        self.computedVis =  parent.computedVis \
            if isinstance(parent, NodeViewItem) \
            else UsdGeom.Tokens.inherited
        if self.imageable and self.active:
            if isVisibilityInherited:
                self.vis = UsdGeom.Tokens.inherited 
            else:
                self.vis = self.computedVis = UsdGeom.Tokens.invisible

    def addChildren(self, children):
        """Adds children to the end of this item.  This is the only
           method clients should call to manage an item's children."""
        self._needsChildrenPopulated = False
        super(NodeViewItem, self).addChildren(children)

    def data(self, column, role):
        # All Qt queries that affect the display of this item (label, font,
        # color, etc) will come through this method.  We set that data
        # lazily because it's expensive to do and (for the most part) Qt
        # only needs the data for items that are visible.
        self.push()

        # We report data directly rather than use set...() and letting
        # super().data() return it because Qt can be pathological during
        # calls to set...() when the item hierarchy is attached to a view,
        # making thousands of calls to data().
        result = None
        if column == 0:
            result = self._nameData(role)
        elif column == 1:
            result = self._typeData(role)
        elif column == 2:
            result = self._visData(role)
        if not result:
            result = super(NodeViewItem, self).data(column, role)
        return result

    def _nameData(self, role):
        if role == QtCore.Qt.DisplayRole:
            return self.name
        elif role == QtCore.Qt.FontRole:
            # Abstract prims are also considered defined; since we want
            # to distinguish abstract defined prims from non-abstract
            # defined prims, we check for abstract first.
            if self.abstract:
                return AbstractPrimFont
            elif not self.defined:
                return OverPrimFont
            else:
                return DefinedPrimFont
        elif role == QtCore.Qt.ForegroundRole:
            if self.isInstance:
                color = InstanceColor
            elif self.hasArcs:
                color = HasArcsColor
            elif self.isInMaster:
                color = MasterColor
            else:
                color = NormalColor
            
            return color if self.active else color.color().darker(HALF_DARKER)
        elif role == QtCore.Qt.ToolTipRole:
            toolTip = 'Prim'
            if len(self.typeName) > 0:
                toolTip = self.typeName + ' ' + toolTip
            if self.isInMaster:
                toolTip = 'Master ' + toolTip
            if not self.defined:
                toolTip = 'Undefined ' + toolTip
            elif self.abstract:
                toolTip = 'Abstract ' + toolTip
            else:
                toolTip = 'Defined ' + toolTip
            if not self.active:
                toolTip = 'Inactive ' + toolTip
            elif self.isInstance:
                toolTip = 'Instanced ' + toolTip
            if self.hasArcs:
                toolTip = toolTip + "<br>Has composition arcs"
            return toolTip
        else:
            return None

    def _typeData(self, role):
        if role == QtCore.Qt.DisplayRole:
            return self.typeName
        else:
            return self._nameData(role)

    def _visData(self, role):
        if role == QtCore.Qt.DisplayRole:
            if self.imageable and self.active:
                return "I" if self.vis == UsdGeom.Tokens.invisible else "V"
            else:
                return ""
        elif role == QtCore.Qt.TextAlignmentRole:
            return QtCore.Qt.AlignCenter
        elif role == QtCore.Qt.FontRole:
            return BoldFont
        elif role == QtCore.Qt.ForegroundRole:
            fgColor = self._nameData(role) 
            if (self.imageable and self.active and
                    self.vis != UsdGeom.Tokens.invisible and
                    self.computedVis == UsdGeom.Tokens.invisible):
                fgColor = fgColor.color().darker()
            return fgColor
        else:
            return None

    def needsChildrenPopulated(self):
        return self._needsChildrenPopulated

    def canChangeVis(self):
        if not self.imageable:
            print "WARNING: The prim <" + str(self.node.GetPath()) + \
                    "> is not imageable. Cannot change visibility."
            return False
        elif self.isInMaster:
            print "WARNING: The prim <" + str(self.node.GetPath()) + \
                   "> is in a master. Cannot change visibility."
            return False
        return True

    def loadVis(self, inheritedVis, visHasBeenAuthored):
        if not (self.imageable and self.active):
            return inheritedVis

        time = self._mainWindow._currentFrame
        # If visibility-properties have changed on the stage, then
        # we must re-evaluate our variability before deciding whether
        # we can avoid re-reading our visibility
        visAttr = UsdGeom.Imageable(self.node).GetVisibilityAttr()
        if visHasBeenAuthored:
            self.visVaries = visAttr.ValueMightBeTimeVarying()
                
            if not self.visVaries:
                self.vis = visAttr.Get(time)

        if self.visVaries:
            self.vis =  visAttr.Get(time)
            
        self.computedVis = UsdGeom.Tokens.invisible \
            if self.vis == UsdGeom.Tokens.invisible \
            else inheritedVis

        self.emitDataChanged()
        return self.computedVis

    def activeChanged(self):
        # We need to completely re-generate the prim tree because making a prim
        # inactive can break a reference chain, and/or make another prim
        # inactive due to inheritance.
        self._mainWindow.UpdateNodeViewContents()

    def loadStateChanged(self):
        # We can do better than nuking the whole prim tree, but for now,
        # use what's already handy
        self._mainWindow.UpdateNodeViewContents()

    @staticmethod
    def propagateVis(item, authoredVisHasChanged=True):
        parent = item.parent()
        inheritedVis = parent._resetAncestorsRecursive(authoredVisHasChanged) \
            if isinstance(parent, NodeViewItem) \
            else UsdGeom.Tokens.inherited
        # This may be called on the "InvisibleRootItem" that is an ordinary
        # QTreeWidgetItem, in which case we need to process its children
        # individually
        if isinstance(item, NodeViewItem):
            item._pushVisRecursive(inheritedVis, authoredVisHasChanged)
        else:
            for child in [item.child(i) for i in xrange(item.childCount())]:
                child._pushVisRecursive(inheritedVis, authoredVisHasChanged)


    def _resetAncestorsRecursive(self, authoredVisHasChanged):
        parent = self.parent()
        inheritedVis = parent._resetAncestorsRecursive(authoredVisHasChanged) \
            if isinstance(parent, NodeViewItem) \
            else UsdGeom.Tokens.inherited

        return self.loadVis(inheritedVis, authoredVisHasChanged)

    def _pushVisRecursive(self, inheritedVis, authoredVisHasChanged):
        myComputedVis = self.loadVis(inheritedVis, authoredVisHasChanged)
            
        for child in [self.child(i) for i in xrange(self.childCount())]:
            child._pushVisRecursive(myComputedVis, authoredVisHasChanged)

    def setActive(self, active):
        if self.isInMaster:
            print "WARNING: The prim <" + str(self.node.GetPath()) + \
                   "> is in a master. Cannot change activation."
            return

        self.node.SetActive(active)
        self.activeChanged()

    def setLoaded(self, loaded):
        if self.node.IsMaster():
            print "WARNING: The prim <" + str(self.node.GetPath()) + \
                   "> is a master prim. Cannot change load state."
            return

        if self.node.IsActive():
            if loaded:
                self.node.Load()
            else:
                self.node.Unload()
            self.loadStateChanged()

    def setVisible(self, visible):
        if self.canChangeVis():
            UsdGeom.Imageable(self.node).GetVisibilityAttr().Set(UsdGeom.Tokens.inherited 
                             if visible else UsdGeom.Tokens.invisible)
            self.visChanged()

    def makeVisible(self):
        if self.canChangeVis():
            # It is in general not kosher to use an Sdf.ChangeBlock around
            # operations at the Usd API level.  We have carefully arranged
            # (with insider knowledge) to have only "safe" mutations
            # happening inside the ChangeBlock.  We do this because
            # UsdImaging updates itself independently for each
            # Usd.Notice.ObjectsChanged it receives.  We hope to eliminate
            # the performance need for this by addressing bug #121992
            from pxr import Sdf
            with Sdf.ChangeBlock():
                UsdGeom.Imageable(self.node).MakeVisible()
            self.visChanged()

    def removeVisibility(self):
        if self.canChangeVis():
            UsdGeom.Imageable(self.node).GetVisibilityAttr().Clear()
            self.visChanged()

    def visChanged(self):
        # called when user authors a new visibility value
        # we must re-determine if visibility is varying over time
        self.loadVis(self.parent().computedVis, True)

    def onClick(self, col):
        """Return True if the click caused the node to change state (visibility,
        etc)"""
        if col == 2 and self.imageable and self.active:
            self.setVisible(self.vis == UsdGeom.Tokens.invisible)
            return True
        return False

