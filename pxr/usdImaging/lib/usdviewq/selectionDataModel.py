#
# Copyright 2017 Pixar
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

from collections import OrderedDict

from pxr import Sdf
from qt import QtCore

from customAttributes import (ComputedPropertyNames, BoundingBoxAttribute,
    LocalToWorldXformAttribute, ComputedPropertyFactory)


# Indicates that all instances of a prim are selected.
ALL_INSTANCES = -1


class Blocker:
    """Object which can be used to temporarily block the execution of a body of
    code. This object is a context manager, and enters a 'blocked' state when
    used in a 'with' statement. The 'blocked()' method can be used to find if
    the Blocker is in this 'blocked' state.
    """

    def __init__(self, exitCallback=lambda: None):

        # A count is used rather than a 'blocked' flag to allow for nested
        # blocking.
        self._count = 0

        # Fired when the Blocker's context is exited.
        self._exitCallback = exitCallback

    def __enter__(self):
        """Enter the 'blocked' state until the context is exited."""

        self._count += 1

    def __exit__(self, *args):
        """Exit the 'blocked' state."""

        self._count -= 1

        if not self.blocked():
            self._exitCallback()

    def blocked(self):
        """Returns True if in the 'blocked' state, and False otherwise."""

        return self._count > 0


class _PrimSelection(object):
    """This class keeps track of the core data for prim selection: paths and
    instances. The methods here can be called in any order required without
    corrupting the path selection state.
    """

    def __init__(self):

        # The order of paths selected needs to be maintained so we can track the
        # focus path. An OrderedDict is more efficient than a list here since it
        # supports efficient removal of arbitrary paths but still maintains the
        # path order. Sdf.Path objects are the keys in the OrderedDict, and a
        # set of selected instances are the values. If all instances are
        # selected, None is the value rather than a set.
        self._selection = OrderedDict()

        self._added = set()
        self._removed = set()

    def _clearPrimPath(self, path):
        """Clears a path from the selection and updates the diff."""

        if path in self._added:
            # Path was added in this diff, but we are removing it again. Since
            # there is no net change, it shouldn't be in _added or _removed.
            self._added.discard(path)
        else:
            self._removed.add(path)

        del self._selection[path]

    def _discardInstance(self, path, instance):
        """Discards an instance from the selection, then deletes the path from
        the selection if it has no more instances.
        """

        instances = self._selection[path]
        instances.discard(instance)
        if len(instances) == 0:
            # Last instance deselected, so path should be deselected.
            self._clearPrimPath(path)

    def _allInstancesSelected(self, path):
        """Returns True if all instances of a specified path are selected and
        False otherwise.
        """

        if path in self._selection:
            return self._selection[path] == ALL_INSTANCES
        else:
            return False

    def _noInstancesSelected(self, path):
        """Returns True if all instances of a specified path are selected and
        False otherwise.
        """

        return path not in self._selection

    def clear(self):
        """Clear the path selection."""

        for path in self._selection.keys():
            self._clearPrimPath(path)

    def addPrimPath(self, path, instance=ALL_INSTANCES):
        """Add a path to the selection. If an instance is given, then only add
        that instance. If all instances are selected when this happens then the
        single instance will become the only selected one.
        """

        # If the path is not already in the selection, update the diff.
        if path not in self._selection:
            if path in self._removed:
                # Path was removed in this diff, but is back now. Since there is
                # no net change, it shouldn't be in _added or _removed.
                self._removed.discard(path)
            else:
                self._added.add(path)

        if instance == ALL_INSTANCES:
            # Trying to add all instances, make sure all instances are selected.
            self._selection[path] = ALL_INSTANCES
        else:
            # Trying to add a single instance.
            if self._allInstancesSelected(path) or self._noInstancesSelected(path):
                # Either all instances selected or none selected. Create an
                # empty set of instances then add the target instance.
                self._selection[path] = set()
            self._selection[path].add(instance)

    def removePrimPath(self, path, instance=ALL_INSTANCES):
        """Remove a path from the selection. If an instance is given, then only
        remove that instance. If all instances are selected when this happens,
        deselect all instances. If the target does not exist in the selection,
        do nothing.
        """

        if path in self._selection:
            if instance == ALL_INSTANCES or self._allInstancesSelected(path):
                # Want to deselect all instances or all instances are selected.
                # Either way, deselect all.
                self._clearPrimPath(path)
            else:
                # Some instances selected and want to deselect one of them.
                self._discardInstance(path, instance)

    def togglePrimPath(self, path, instance=ALL_INSTANCES):
        """Toggle the selection of a path. If an instance is given, only toggle
        that instance's selection.
        """

        if path not in self._selection:
            self.addPrimPath(path, instance)
            return

        # Path is at least partially selected.
        if instance == ALL_INSTANCES:
            # Trying to toggle all instances.
            if self._allInstancesSelected(path):
                # All instances are already selected, so deselect the path.
                self._clearPrimPath(path)
            else:
                # Only some instances are selected, select all all of them.
                self._selection[path] = ALL_INSTANCES
        else:
            # Trying to toggle a single instance.
            if self._allInstancesSelected(path):
                # Currently all instances are selected. Switch selection to
                # only the new instance.
                self._selection[path] = set([instance])
            else:
                # Some instances already selected. Toggle the new instance
                # in the selection.
                instances = self._selection[path]
                if instance in instances:
                    self._discardInstance(path, instance)
                else:
                    instances.add(instance)

    def getPrimPaths(self):
        """Get a list of paths that are at least partially selected."""

        return list(self._selection.keys())

    def getPrimPathInstances(self):
        """Get the full selection of paths and their corresponding selected
        instances.
        """

        return OrderedDict(
            (path, set(instances)) if isinstance(instances, set) else (path, instances)
            for path, instances in self._selection.items())

    def getDiff(self):
        """Get the prims added to or removed from the selection since the last
        time getDiff() was called.
        """

        diff = (self._added, self._removed)

        self._added = set()
        self._removed = set()

        return diff


class _PropSelection(object):
    """This class keeps track of the state of property selection."""

    def __init__(self):

        self._selection = OrderedDict()

    def clear(self):
        """Clears the property selection."""

        self._selection = OrderedDict()

    def addPropPath(self, primPath, propName):
        """Add a property to the selection."""

        propTuple = (primPath, propName)

        # If this property is already selected, remove it from the selection and
        # re-add it so it becomes the focus property.
        if propTuple in self._selection:
            targets = self._selection[propTuple]
            del self._selection[propTuple]
            self._selection[propTuple] = targets
        else:
            self._selection[propTuple] = set()

    def removePropPath(self, primPath, propName):
        """Remove a property from the selection."""

        propTuple = (primPath, propName)
        if propTuple in self._selection:
            del self._selection[propTuple]

    def addTarget(self, primPath, propName, target):
        """Add a target to the selection. Also add the target's property if it
        is not already in the selection.
        """

        propTuple = (primPath, propName)

        # If this property is already selected, remove it from the selection and
        # re-add it so it becomes the focus property.
        if propTuple in self._selection:
            targets = self._selection[propTuple]
            del self._selection[propTuple]
            self._selection[propTuple] = targets
        else:
            targets = self._selection.setdefault(propTuple, set())

        targets.add(target)

    def removeTarget(self, primPath, propName, target):
        """Remove a target from the selection. If the target or its property are
        not already in the selection, nothing is changed.
        """

        propTuple = (primPath, propName)
        if propTuple in self._selection:
            self._selection[propTuple].discard(target)

    def getPropPaths(self):
        """Get the list of properties."""

        return list(self._selection.keys())

    def getTargets(self):
        """Get a dictionary which maps selected properties to a set of their
        selected targets or connections.
        """

        propTargets = OrderedDict()
        for propTuple, targets in self._selection.items():
            propTargets[propTuple] = set(targets)

        return propTargets


class SelectionDataModel(QtCore.QObject):
    """Data model managing the current selection of prims and properties."""

    # Signals must be declared in the class, but each instance of the selection
    # data model has its own unique signal instances.

    # When emitted, includes two sets: one of newly selected prims, and one of
    # newly deselected prims.
    signalPrimSelectionChanged = QtCore.Signal(set, set)

    signalPropSelectionChanged = QtCore.Signal()
    signalComputedPropSelectionChanged = QtCore.Signal()

    def __init__(self, rootDataModel, _computedPropFactory=None):
        QtCore.QObject.__init__(self)

        self._rootDataModel = rootDataModel

        # _computedPropFactory may be passed explicitly for unit testing.
        if _computedPropFactory is None:
            self._computedPropFactory = ComputedPropertyFactory(
                self._rootDataModel)
        else:
            self._computedPropFactory = _computedPropFactory

        self.batchPrimChanges = Blocker(
            exitCallback=self._primSelectionChanged)
        self.batchPropChanges = Blocker(
            exitCallback=self._propSelectionChanged)
        self.batchComputedPropChanges = Blocker(
            exitCallback=self._computedPropSelectionChanged)

        self._primSelection = _PrimSelection()

        # The path selection should never be empty. If it ever is, we
        # immediately add the root path before returning to user control (see
        # _primSelectionChanged).
        self._primSelection.addPrimPath(Sdf.Path.absoluteRootPath)
        # Clear the prim selection diff so we don't get the absolute root in the
        # first signal from signalPrimSelectionChanged.
        self._primSelection.getDiff()

        self._lcdPathSelection = [Sdf.Path.absoluteRootPath]

        self._propSelection = _PropSelection()
        self._computedPropSelection = _PropSelection()

    ### Internal Operations ###

    def _primSelectionChanged(self):
        """Should be called whenever a change is made to _primSelection. Some
        final work is done then the prim selection changed signal is emitted.
        """

        # If updates are suppressed, do not emit a signal or do any
        # pre-processing.
        if self.batchPrimChanges.blocked():
            return

        # Make sure there is always at least one path selected.
        if len(self.getPrimPaths()) == 0:
            self._primSelection.addPrimPath(Sdf.Path.absoluteRootPath)

        # Recalculate the LCD prims whenever the path selection changes.
        paths = self._primSelection.getPrimPaths()
        if len(paths) > 1:
            paths = [path for path in paths
                        if path != Sdf.Path.absoluteRootPath]
        self._lcdPathSelection = Sdf.Path.RemoveDescendentPaths(paths)

        # Finally, emit the changed signal.
        added, removed = self._primSelection.getDiff()
        self.signalPrimSelectionChanged.emit(added, removed)

    def _propSelectionChanged(self):
        """Should be called whenever a change is made to _propSelection."""

        # If updates are suppressed, do not emit a signal or do any
        # pre-processing.
        if self.batchPropChanges.blocked():
            return

        self.signalPropSelectionChanged.emit()

    def _computedPropSelectionChanged(self):
        """Should be called whenever a change is made to _computedPropSelection.
        """

        # If updates are suppressed, do not emit a signal or do any
        # pre-processing.
        if self.batchComputedPropChanges.blocked():
            return

        self.signalComputedPropSelectionChanged.emit()

    def _ensureValidPrimPath(self, path):
        """Validate an input path. If it is a string path, convert it to an
        Sdf.Path object.
        """

        sdfPath = Sdf.Path(str(path))
        if not sdfPath.IsAbsoluteRootOrPrimPath():
            raise ValueError("Path must be a prim path, got: {}".format(
                repr(sdfPath)))
        return sdfPath

    def _validateInstanceIndexParameter(self, instance):
        """Validate an instance used as a parameter. This can be any positive
        int or ALL_INSTANCES."""

        validIndex = False
        if isinstance(instance, int):
            if instance >= 0 or instance == ALL_INSTANCES:
                validIndex = True

        if not validIndex:
            raise ValueError(
                "Instance must be a positive int or ALL_INSTANCES"
                ", got: {}".format(repr(instance)))

    def _ensureValidPropPath(self, prop):
        """Validate a property."""

        sdfPath = Sdf.Path(str(prop))
        if not sdfPath.IsPropertyPath():
            raise ValueError("Path must be a property path, got: {}".format(
                repr(sdfPath)))
        return sdfPath

    def _ensureValidTargetPath(self, target):
        """Validate a property target or connection."""

        return Sdf.Path(str(target))

    def _getPropFromPath(self, path):
        """Get a Usd property object from a property path."""

        prim = self._rootDataModel.stage.GetPrimAtPath(path.GetPrimPath())
        return prim.GetProperty(path.name)

    def _getTargetFromPath(self, path):
        """Get the Usd object from a target path. It can be either a Usd prim or
        Usd property.
        """

        if path.IsPropertyPath():
            return self._getPropFromPath(path)
        else:
            return self._rootDataModel.stage.GetPrimAtPath(path)

    def _requireNotBatchingPrims(self):
        """Raise an error if we are currently batching prim selection changes.
        We don't want to allow reading prim selection state in the middle of a
        batch.
        """

        if self.batchPrimChanges.blocked():
            raise RuntimeError(
                "Cannot get prim selection state while batching changes.")

    def _requireNotBatchingProps(self):
        """Raise an error if we are currently batching prop selection changes.
        We don't want to allow reading prop selection state in the middle of a
        batch.
        """

        if self.batchPropChanges.blocked():
            raise RuntimeError(
                "Cannot get property selection state while batching changes.")

    def _getComputedPropFromPath(self, primPath, propName):
        """Get a CustomAttribute object from a prim path and property name.
        Raise an error if the property name does not match any known
        CustomAttribute.
        """

        prim = self._rootDataModel.stage.GetPrimAtPath(primPath)
        return self._computedPropFactory.getComputedProperty(prim, propName)

    def _requireNotBatchingComputedProps(self):
        """Raise an error if we are currently batching prop selection changes.
        We don't want to allow reading prop selection state in the middle of a
        batch.
        """

        if self.batchComputedPropChanges.blocked():
            raise RuntimeError("Cannot get computed property selection state "
                "while batching changes.")

    def _buildPropPath(self, primPath, propName):
        """Build a new property path from a prim path and a property name."""

        return Sdf.Path(str(primPath) + "." + propName)

    def _validateComputedPropName(self, propName):
        """Validate a computed property name."""

        if propName not in ComputedPropertyNames:
            raise ValueError("Invalid computed property name: {}".format(
                repr(propName)))

    def _switchProps(self, fromPrimPath, toPrimPath):
        """Switch all selected properties from one prim to another. Only do this
        if all properties currently belong to the "from" prim.
        """

        propTargets = self.getPropTargetPaths()
        computedProps = self.getComputedPropPaths()

        # Check that all properties belong to the "from" prim.
        for propPath in propTargets:
            if propPath.GetPrimPath() != fromPrimPath:
                return
        for propPrimPath, propName in computedProps:
            if propPrimPath != fromPrimPath:
                return

        # Switch all properties to the "to" prim.
        with self.batchPropChanges:

            self.clearProps()

            # Root prim cannot have non-computed properties. The property paths
            # in this case are invalid.
            if str(toPrimPath) != "/":
                for propPath, targets in propTargets.items():
                    newPropPath = self._buildPropPath(toPrimPath, propPath.name)
                    self.addPropPath(newPropPath)
                    for target in targets:
                        self.addPropTargetPath(newPropPath, target)

        with self.batchComputedPropChanges:

            self.clearComputedProps()

            for primPath, propName in computedProps:
                self.addComputedPropPath(toPrimPath, propName)

    ### General Operations ###

    def clear(self):
        """Clear all selections."""

        self.clearPrims()
        self.clearProps()

    ### Prim Path Operations ###

    def clearPrims(self):
        """Clear the prim selection (same as path selection)."""

        self._primSelection.clear()
        self._primSelectionChanged()

    def addPrimPath(self, path, instance=ALL_INSTANCES):
        """Add a path to the path selection. If an instance is given, only add
        that instance.
        """

        path = self._ensureValidPrimPath(path)
        self._validateInstanceIndexParameter(instance)

        self._primSelection.addPrimPath(path, instance)
        self._primSelectionChanged()

    def removePrimPath(self, path, instance=ALL_INSTANCES):
        """Remove a path from the path selection. If an instance is given, only
        remove that instance. If the target does not exist in the selection, do
        nothing.
        """

        path = self._ensureValidPrimPath(path)
        self._validateInstanceIndexParameter(instance)

        self._primSelection.removePrimPath(path, instance)
        self._primSelectionChanged()

    def togglePrimPath(self, path, instance=ALL_INSTANCES):
        """Toggle a path in the path selection. If an instance is given, only
        that instance is toggled.
        """

        path = self._ensureValidPrimPath(path)
        self._validateInstanceIndexParameter(instance)

        self._primSelection.togglePrimPath(path, instance)
        self._primSelectionChanged()

    def setPrimPath(self, path, instance=ALL_INSTANCES):
        """Clear the prim selection then add a single prim path back to the
        selection. If an instance is given, only add that instance.
        """

        with self.batchPrimChanges:
            self.clearPrims()
            self.addPrimPath(path, instance)

    def getFocusPrimPath(self):
        """Get the path currently in focus."""

        self._requireNotBatchingPrims()
        return self._primSelection.getPrimPaths()[0]

    def getPrimPaths(self):
        """Get a list of all selected paths."""

        self._requireNotBatchingPrims()
        return self._primSelection.getPrimPaths()

    def getLCDPaths(self):
        """Get a list of paths from the selection who do not have an ancestor
        that is also in the selection. The "Least Common Denominator" paths.
        """

        self._requireNotBatchingPrims()
        return list(self._lcdPathSelection)

    def getPrimPathInstances(self):
        """Get a dictionary which maps each selected prim to a set of its
        selected instances. If all of a path's instances are selected, the value
        is ALL_INSTANCES rather than a set.
        """

        self._requireNotBatchingPrims()
        return self._primSelection.getPrimPathInstances()

    def switchToPrimPath(self, path, instance=ALL_INSTANCES):
        """Select only the given prim path. If only a single prim was selected
        before and all selected properties belong to this prim, select the
        corresponding properties on the new prim instead. If an instance is
        given, only select that instance.
        """

        path = self._ensureValidPrimPath(path)

        oldPrimPaths = self.getPrimPaths()

        with self.batchPrimChanges:
            self.clearPrims()
            self.addPrimPath(path, instance)

        if len(oldPrimPaths) == 1:
            self._switchProps(oldPrimPaths[0], path)

    ### Prim Operations ###
    # These are all convenience methods which just call their respective path
    # operations.

    def addPrim(self, prim, instance=ALL_INSTANCES):
        """Add a prim's path to the path selection. If an instance is given,
        only add that instance.
        """

        self.addPrimPath(prim.GetPath(), instance)

    def removePrim(self, prim, instance=ALL_INSTANCES):
        """Remove a prim from the prim selection. If an instance is given, only
        remove that instance. If the target does not exist in the selection, do
        nothing.
        """

        self.removePrimPath(prim.GetPath(), instance)

    def togglePrim(self, prim, instance=ALL_INSTANCES):
        """Toggle a prim's path in the path selection. If an instance is given,
        only that instance is toggled.
        """

        self.togglePrimPath(prim.GetPath(), instance)

    def setPrim(self, prim, instance=ALL_INSTANCES):
        """Clear the prim selection then add a single prim back to the
        selection. If an instance is given, only add that instance.
        """

        self.setPrimPath(prim.GetPath(), instance)

    def getFocusPrim(self):
        """Get the prim whose path is currently in focus."""

        return self._rootDataModel.stage.GetPrimAtPath(self.getFocusPrimPath())

    def getPrims(self):
        """Get a list of all prims whose paths are selected."""

        return [self._rootDataModel.stage.GetPrimAtPath(path)
            for path in self.getPrimPaths()]

    def getLCDPrims(self):
        """Get a list of prims whose paths are both selected and do not have an
        ancestor that is also in the selection. The "Least Common Denominator"
        prims.
        """

        return [self._rootDataModel.stage.GetPrimAtPath(path)
            for path in self.getLCDPaths()]

    def getPrimInstances(self):
        """Get a dictionary which maps each prim whose path is selected to a set
        of its selected instances. If all of a path's instances are selected,
        the value is ALL_INSTANCES rather than a set.
        """

        return OrderedDict(
            (self._rootDataModel.stage.GetPrimAtPath(path), instance)
            for path, instance in self.getPrimPathInstances().items())

    def switchToPrim(self, prim, instance=ALL_INSTANCES):
        """Select only the given prim. If only a single prim was selected before
        and all selected properties belong to this prim, select the
        corresponding properties on the new prim instead.
        """

        self.switchToPrimPath(prim.GetPath(), instance)

    ### Property Path Operations ###

    def clearProps(self):
        """Clear the property selection."""

        self._propSelection.clear()
        self._propSelectionChanged()

    def addPropPath(self, path):
        """Add a property to the selection."""

        path = self._ensureValidPropPath(path)

        primPath = path.GetPrimPath()
        propName = path.name

        self._propSelection.addPropPath(primPath, propName)
        self._propSelectionChanged()

    def removePropPath(self, path):
        """Remove a property from the selection."""

        path = self._ensureValidPropPath(path)

        primPath = path.GetPrimPath()
        propName = path.name

        self._propSelection.removePropPath(primPath, propName)
        self._propSelectionChanged()

    def setPropPath(self, path):
        """Clear the property selection, then add a single property path back to
        the selection.
        """

        path = self._ensureValidPropPath(path)

        with self.batchPropChanges:
            self.clearProps()
            self.addPropPath(path)

    def addPropTargetPath(self, path, targetPath):
        """Select a property's target or connection."""

        path = self._ensureValidPropPath(path)
        targetPath = self._ensureValidTargetPath(targetPath)

        primPath = path.GetPrimPath()
        propName = path.name

        self._propSelection.addTarget(primPath, propName, targetPath)
        self._propSelectionChanged()

    def removePropTargetPath(self, path, targetPath):
        """Deselect a property's target or connection."""

        path = self._ensureValidPropPath(path)
        targetPath = self._ensureValidTargetPath(targetPath)

        primPath = path.GetPrimPath()
        propName = path.name

        self._propSelection.removeTarget(primPath, propName, targetPath)
        self._propSelectionChanged()

    def setPropTargetPath(self, path, targetPath):
        """Clear the property selection, then add a single property path back to
        the selection with a target.
        """

        with self.batchPropChanges:
            self.clearProps()
            self.addPropTargetPath(path, targetPath)

    def getFocusPropPath(self):
        """Get the focus property from the property selection."""

        self._requireNotBatchingProps()

        propPaths = [self._buildPropPath(*propTuple)
            for propTuple in self._propSelection.getPropPaths()]
        if len(propPaths) > 0:
            return propPaths[-1]
        else:
            return None

    def getPropPaths(self):
        """Get a list of all selected properties."""

        self._requireNotBatchingProps()

        propPaths = [self._buildPropPath(*propTuple)
            for propTuple in self._propSelection.getPropPaths()]
        return propPaths

    def getPropTargetPaths(self):
        """Get a dictionary which maps selected properties to a set of their
        selected targets or connections.
        """

        self._requireNotBatchingProps()

        return OrderedDict((self._buildPropPath(*propTuple), set(targets))
            for propTuple, targets in self._propSelection.getTargets().items())

    ### Property Operations ###

    def addProp(self, prop):
        """Add a property to the selection."""

        self.addPropPath(prop.GetPath())

    def removeProp(self, prop):
        """Remove a property from the selection."""

        self.removePropPath(prop.GetPath())

    def setProp(self, prop):
        """Clear the property selection, then add a single property back to the
        selection.
        """

        self.setPropPath(prop.GetPath())

    def addPropTarget(self, prop, target):
        """Select a property's target or connection."""

        self.addPropTargetPath(prop.GetPath(), target.GetPath())

    def removePropTarget(self, prop, target):
        """Deselect a property's target or connection."""

        self.removePropTargetPath(prop.GetPath(), target.GetPath())

    def setPropTarget(self, prop, target):
        """Clear the property selection, then add a single property back to the
        selection with a target.
        """

        self.removePropTargetPath(prop.GetPath(), target.GetPath())

    def getFocusProp(self):
        """Get the focus property from the property selection."""

        focusPath = self.getFocusPropPath()
        if focusPath is None:
            return None

        return self._getPropFromPath(focusPath)

    def getProps(self):
        """Get a list of all selected properties."""

        return [self._getPropFromPath(path)
            for path in self.getPropPaths()]

    def getPropTargets(self):
        """Get a dictionary which maps selected properties to a set of their
        selected targets or connections.
        """

        propTargets = OrderedDict()
        for propPath, targetPaths in self.getPropTargetPaths().items():

            prop = self._getPropFromPath(propPath)
            targets = {self._getTargetFromPath(target)
                for target in targetPaths}

            propTargets[prop] = targets

        return propTargets

    ### Computed Property Path Operations ###

    def clearComputedProps(self):
        """Clear the computed property selection."""

        self._computedPropSelection.clear()
        self._computedPropSelectionChanged()

    def addComputedPropPath(self, primPath, propName):
        """Add a computed property to the selection."""

        primPath = self._ensureValidPrimPath(primPath)
        self._validateComputedPropName(propName)

        self._computedPropSelection.addPropPath(primPath, propName)
        self._computedPropSelectionChanged()

    def removeComputedPropPath(self, primPath, propName):
        """Remove a computed property from the selection."""

        primPath = self._ensureValidPrimPath(primPath)
        self._validateComputedPropName(propName)

        self._computedPropSelection.removePropPath(primPath, propName)
        self._computedPropSelectionChanged()

    def setComputedPropPath(self, primPath, propName):
        """Clear the computed property selection, then add a single computed
        property path back to the selection.
        """

        primPath = self._ensureValidPrimPath(primPath)
        self._validateComputedPropName(propName)

        with self.batchComputedPropChanges:
            self.clearComputedProps()
            self.addComputedPropPath(primPath, propName)

    def getFocusComputedPropPath(self):
        """Get the focus computed property from the property selection."""

        self._requireNotBatchingComputedProps()

        propPaths = self._computedPropSelection.getPropPaths()
        if len(propPaths) > 0:
            return propPaths[-1]
        else:
            return (None, None)

    def getComputedPropPaths(self):
        """Get a list of all selected computed properties."""

        self._requireNotBatchingComputedProps()

        return self._computedPropSelection.getPropPaths()

    ### Computed Property Operations ###

    def addComputedProp(self, prop):
        """Add a computed property to the selection."""

        self.addComputedPropPath(prop.GetPrimPath(), prop.GetName())

    def removeComputedProp(self, prop):
        """Remove a computed property from the selection."""

        self.removeComputedPropPath(prop.GetPrimPath(), prop.GetName())

    def setComputedProp(self, prop):
        """Clear the computed property selection, then add a single computed
        property back to the selection.
        """

        self.setComputedPropPath(prop.GetPrimPath(), prop.GetName())

    def getFocusComputedProp(self):
        """Get the focus computed property from the property selection."""

        focusPath = self.getFocusComputedPropPath()
        if focusPath == (None, None):
            return None

        return self._getComputedPropFromPath(*focusPath)

    def getComputedProps(self):
        """Get a list of all selected computed properties."""

        return [self._getComputedPropFromPath(*path)
            for path in self.getComputedPropPaths()]
