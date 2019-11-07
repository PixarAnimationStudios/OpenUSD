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

from __future__ import print_function
import os, sys, json


class _StateProp(object):
    """Defines a state property on a StateSource object."""

    def __init__(self, name, default, propType, validator):
        self.name = name
        self.default = default
        self.propType = propType
        self.validator = validator


class StateSource(object):
    """An object which has some savable application state."""

    def __init__(self, parent, name):
        self._parentStateSource = parent
        self._childStateSources = dict()
        self._stateSourceName = name
        self._stateSourceProperties = dict()

        # Register child source with the parent.
        if self._parentStateSource is not None:
            self._parentStateSource._registerChildStateSource(self)

    def _registerChildStateSource(self, child):
        """Registers a child StateSource with this source object."""
        self._childStateSources[child._stateSourceName] = child

    def _getState(self):
        """Get this source's state dict from its parent source."""
        if self._parentStateSource is None:
            return dict()
        else:
            return self._parentStateSource._getChildState(self._stateSourceName)

    def _getChildState(self, childName):
        """Get a child source's state dict. This method guarantees that a dict
        will be return but does not guarantee anything about the contents of
        the dict.
        """
        state = self._getState()

        if childName in state:
            childState = state[childName]
            # Child state could be loaded from file as any JSON-serializable
            # type (int, str, etc.). Only return it if it is a dict. Otherwise,
            # fallback to empty dict.
            if isinstance(childState, dict):
                return childState

        # Create a new state dict for the child and save it in this source's
        # state dict.
        childState = dict()
        state[childName] = childState
        return childState

    def _typeCheck(self, value, prop):
        """Validate a value against a StateProp."""

        # Make sure the value has the correct type.
        valueType = type(value)
        if valueType is not prop.propType:
            if valueType is int and prop.propType is float:
                pass # ints are valid for float types.
            elif prop.propType in (str, unicode) and valueType in (str, unicode):
                pass # str and unicode can be used interchangeably.
            else:
                print("Value {} has type {} but state property {} has type {}.".format(
                        repr(value), valueType, repr(prop.name), prop.propType),
                        file=sys.stderr)
                print("    Using default value {}.".format(repr(prop.default)),
                        file=sys.stderr)
                return False

        # Make sure value passes custom validation. Otherwise, use default value.
        if prop.validator(value):
            return True
        else:
            print("Value {} did not pass custom validation for state property {}.".format(
                    repr(value), repr(prop.name)), file=sys.stderr)
            print("    Using default value {}.".format(repr(prop.default)),
                    file=sys.stderr)
            return False

    def _saveState(self):
        """Saves the source's state to the settings object's state buffer."""
        newState = dict()

        # Save state properties.
        self.onSaveState(newState)

        # Validate state properties.
        for name, value in tuple(newState.items()):
            if name not in self._stateSourceProperties:
                print("State property {} not defined. It will be removed.".format(
                        repr(name)), file=sys.stderr)
                del newState[name]
            prop = self._stateSourceProperties[name]
            if self._typeCheck(value, prop):
                newState[name] = value
            else:
                newState[name] = prop.default

        # Make sure no state properties were forgotten.
        for prop in self._stateSourceProperties.values():
            if prop.name not in newState:
                print("State property {} not saved.".format(repr(prop.name)),
                        file=sys.stderr)

        # Update the real state dict with the new state. This preserves unused
        # data loaded from the state file.
        self._getState().update(newState)

        # Save all child states.
        for child in self._childStateSources.values():
            child._saveState()

    def stateProperty(self, name, default, propType=None, validator=lambda value: True):
        """Validates and creates a new StateProp for this source. The property's
        value is returned so this method can be used during StateSource
        initialization."""

        # Make sure there are no conflicting state properties.
        if name in self._stateSourceProperties:
            raise RuntimeError("State property name {} already in use.".format(
                    repr(name)))

        # Grab state property type from default value if it was not defined.
        if propType is None:
            propType = type(default)

        # Make sure default value is valid.
        if not isinstance(default, propType):
            raise RuntimeError("Default value {} does not match type {}.".format(
                    repr(default), repr(propType)))
        if not validator(default):
            raise RuntimeError("Default value {} does not pass custom validation "
                    "for state property {}.".format(repr(default), repr(name)))

        prop = _StateProp(name, default, propType, validator)
        self._stateSourceProperties[name] = prop

        # Load the value from the state dict and validate it.
        state = self._getState()
        value = state.get(name, default)
        if self._typeCheck(value, prop):
            return value
        else:
            return prop.default

    def onSaveState(self, state):
        """Save the source's state properties to a dict."""
        raise NotImplementedError


class Settings(StateSource):
    """An object which encapsulates saving and loading of application state to
    a state file. When created, it loads state from a state file and stores it
    in a buffer. Its children sources can fetch their piece of state from the
    buffer. On save, this object tells its children to save their current
    states, then saves the buffer back to the state file.
    """

    def __init__(self, version, stateFilePath=None):
        # Settings should be the only StateSource with no parent or name.
        StateSource.__init__(self, None, None)
        self._version = version
        self._stateFilePath = stateFilePath
        self._versionsStateBuffer = None
        self._stateBuffer = None
        self._isEphemeral = (self._stateFilePath is None)

        self._loadState()

    def _loadState(self):
        """Loads and returns application state from a state file. If the file is
        not found, contains invalid JSON, does not contain a dictionary, an
        empty state is returned instead.
        """

        # Load the dict containing all versions of app state.
        if not self._isEphemeral:
            try:
                with open(self._stateFilePath, "r") as fp:
                    self._versionsStateBuffer = json.load(fp)
            except IOError as e:
                if os.path.isfile(self._stateFilePath):
                    print("Error opening state file: " + str(e), file=sys.stderr)
                else:
                    print("State file not found, a new one will be created.",
                            file=sys.stderr)
            except ValueError:
                print("State file contained invalid JSON. Please fix or delete " +
                        "it. Default settings will be used for this instance of " +
                        "USDView, but will not be saved.", file=sys.stderr)
                self._isEphemeral = True

        # Make sure JSON returned a dict.
        if not isinstance(self._versionsStateBuffer, dict):
            self._versionsStateBuffer = dict()

        # Load the correct version of the state dict.
        self._stateBuffer = self._versionsStateBuffer.get(self._version, None)
        if not isinstance(self._stateBuffer, dict):
            self._stateBuffer = dict()
            self._versionsStateBuffer[self._version] = self._stateBuffer

    # overrides StateSource._getState
    def _getState(self):
        """Gets the buffered state rather than asking its parent for its state.
        """
        return self._stateBuffer

    def save(self):
        """Inform all children to save their states, then write the state buffer
        back to the state file.
        """
        if not self._isEphemeral:
            self._saveState()
            try:
                with open(self._stateFilePath, "w") as fp:
                    json.dump(self._versionsStateBuffer, fp,
                            indent=2, separators=(",", ": "))
            except IOError as e:
                print("Could not save state file: " + str(e), file=sys.stderr)

    def onSaveState(self, state):
        """Settings object has no state properties."""
        pass
