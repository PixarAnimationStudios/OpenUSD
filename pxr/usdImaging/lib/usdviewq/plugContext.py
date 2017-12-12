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

# Updates to the plugin API should be made in the _PlugContextInternal class.
# All methods which do *not* begin with an underscore will be available for
# plugins to call from a PlugContext object.
# This class should be the only object that knows about the details of the
# UsdView appController.  If that API ever changes, we should only need to update
# this class.
class _PlugContextInternal(object):
    '''
    This class is an interface that provides access to usdview context for
    usdview plugins.  It abstracts away the implementation of usdview so that
    the core can change without affecting plugins.
    '''

    def __init__(self, appController):
        self._appController = appController

    def GetQMainWindow(self):
        ''' Returns a QWidget object that other widgets can use as a parent. '''

        return self._appController._mainWindow

    def GetUsdStage(self):
        ''' Returns the current Usd stage. '''

        return self._appController._rootDataModel.stage

    def GetCurrentGfCamera(self):
        ''' Returns the last computed Gf Camera. '''
        if self._appController._stageView:
            return self._appController._stageView.gfCamera
        else:
            return None

    def GetCurrentFrame(self):
        ''' Returns the current frame. '''

        return self._appController._currentFrame

    def GetModelsFromSelection(self):
        ''' Returns selected models.  this will walk up to find the nearest model.
        Note, this may return "group"'s if they are selected. '''

        models = []
        items = self._appController.getSelectedItems()
        for item in items:
            currItem = item
            while currItem and not currItem.prim.IsModel():
                currItem = currItem.parent()
            if currItem:
                models.append(currItem.prim)

        return models

    def GetSelectedPrimsOfType(self, schemaType):
        ''' Returns selected prims of the provided schemaType (TfType).'''
        prims = []
        items = self._appController.getSelectedItems()
        for item in items:
            if item.prim.IsA(schemaType):
                prims.append(item.prim)

        return prims

    def GetSelectedPrims(self):
        ''' Returns the current prims. '''

        return self._appController._currentPrims

    def GetSelectedPaths(self):
        ''' Returns the paths for the current selections. '''

        return [item.prim.GetPath()
                for item in self._appController.getSelectedItems()]

    def GetConfigDir(self):
        ''' Returns the config dir, typically ~/.usdview/. '''

        return self._appController._outputBaseDirectory()

    def GetInputFilePath(self):
        ''' Returns the file that usdview was called on. '''

        return self._appController._parserData.usdFile

    def PrintStatus(self, msg):
        ''' Prints a status message. '''

        self._appController.statusMessage(msg)

    def GetStageView(self):
        ''' Returns the stageView object. '''

        return self._appController._stageView

    def GetSettings(self):
        ''' Returns settings object. '''

        return self._appController._settings

    # Screen capture functionality.
    def GrabWindowShot(self):
        ''' Returns a QImage of the full usdview window. '''

        return self._appController.GrabWindowShot()

    def GrabViewportShot(self):
        ''' Returns a QImage of the current stage view in usdview. '''

        return self._appController.GrabViewportShot()


# The following code proxies the methods from _PlugContextInternal and
# preserves names/documentation so the help() function displays useful
# information about both a PlugContext object and each of its proxy methods.


def _apiMethodFilter(name):
    '''Returns True if the property name belongs to a plugin API method from
    _PlugContextInternal.
    '''
    value = getattr(_PlugContextInternal, name)
    return isinstance(value, types.MethodType) and not name.startswith("_")


# Find all API methods on _PlugContextInternal.
_apiMethodNames = filter(_apiMethodFilter, dir(_PlugContextInternal))


class _PlugContextMeta(type):

    def __new__(metacls, cls, bases, classdict):
        '''Create a new PlugContext class with placeholder lambdas for all API
        methods. These placeholders will be replaced with proxy lambdas when a
        PlugContext is created. Placeholders are added so help(plugCtx) displays
        all methods and their documentation.
        '''

        for internalName in _apiMethodNames:
            method = getattr(_PlugContextInternal, internalName)

            def placeholder(self, *args, **kwargs):
                raise NotImplemented
            placeholder.__name__ = method.__name__
            placeholder.__doc__ = method.__doc__

            classdict[internalName] = placeholder

        return type.__new__(metacls, cls, bases, classdict)

    def __dir__(self):
        '''Only return a list of the API method names.'''
        return list(_apiMethodNames)


class PlugContext:
    __doc__ = _PlugContextInternal.__doc__
    __metaclass__ = _PlugContextMeta

    def __init__(self, appController):

        # Create a new _PlugContextInternal object, but do *not* save it as a
        # member variable. Otherwise, plugins could easily access it, defeating
        # the purpose of the proxy.
        plugContext = _PlugContextInternal(appController)

        # Proxies method calls to hide important data from plugins (such as the
        # im_self property on methods). The method itself is still stored in the
        # lambda's closure, but this makes it much more difficult to access it.
        # Method name and documentation are also copied to the lambda so
        # help(plugCtx.<API method>) displays the method name and documentation.
        def proxied(f):
            proxy = lambda *args, **kwargs: f(*args, **kwargs)
            proxy.__name__ = f.__name__
            proxy.__doc__ = f.__doc__
            return proxy

        # Search for and create proxies for all methods whose names do not begin
        # with an underscore.
        for name in _apiMethodNames:
            self.__dict__[name] = proxied(getattr(plugContext, name))
