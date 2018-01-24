#
# Copyright 2018 Pixar
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

import importlib

from pxr import Tf
from pxr import Plug

from qt import QtGui


class DeferredImport(object):
    """Defers importing a module until one of the target callable objects is
    called for the first time. Note that there is no way to know if a callable
    object exists in the target module or even if the target module exists until
    import time. All objects that are referenced are assumed to exist until
    proven otherwise when they are called (at which point an ImportError is
    raised).

    Example:

    math = DeferredImport("math")

    # You can pull as many callable objects from `math` as desired, even if they
    # don't actually exist in `math`.
    sqrt = math.sqrt
    cos = math.cos
    foo = math.foo # does not exist in the real `math` module

    # The `math` module will only be imported when this next line runs because
    # this is the first invocation of a callable object from `math`.
    cos(0)

    # This will raise an ImportError because `math.foo` doesn't really exist.
    foo(0)
    """

    def __init__(self, moduleName, packageName=None):

        self._moduleName = moduleName
        self._packageName = packageName

        self._module = None

    def __getattr__(self, attr):
        """Returns a function which calls the target function of the module and
        passes along any parameters. The module is lazy-imported when a function
        returned by this method is called for the first time.
        """

        def f(*args, **kwargs):

            if self._module is None:

                # Try to import the target module.
                try:
                    self._module = importlib.import_module(
                        self._moduleName, package=self._packageName)
                except ImportError:
                    raise ImportError(
                        "Failed deferred import: module '{}' not found.".format(
                            self._moduleName))

            # Try to get the target function from the imported module.
            try:
                moduleFunction = getattr(self._module, attr)
            except AttributeError:
                raise ImportError(("Failed deferred import: callable object "
                    " '{}' from module '{}' not found").format(
                        attr, self._moduleName))

            # Module and function loaded successfully. Now we can call the
            # function and pass it the parameters.
            return moduleFunction(*args, **kwargs)

        # Return the deferring function. It will be called at some point after
        # this method returns.
        return f


class PluginContainer(object):
    """A base class for a container which holds some Usdview plugins. Specific
    containers should inherit from this class and define the 'registerPlugins'
    and 'configureView' methods.
    """

    def __init__(self, usdviewApi, plugRegistry):

        self._usdviewApi = usdviewApi
        self._plugRegistry = plugRegistry

    def deferredImport(self, moduleName):
        """Return a DeferredImport object which can be used to lazy load
        functions when they are invoked for the first time.
        """

        return DeferredImport(moduleName, self.__module__)

    def registerPlugins(self):
        """This method is called after a container is discovered by Usdview, and
        should call 'registerCommandPlugin' one or more times to add commands to
        Usdview.
        """

        raise NotImplementedError

    def registerCommandPlugin(self, name, callback):
        """Register a single command plugin with Usdview."""

        return self._plugRegistry.registerCommandPlugin(name, callback)

    def configureView(self):
        """This method is called directly after 'registerPlugins' and can be
        used to add menus which invoke a plugin command.
        """

        raise NotImplementedError

    def getMenu(self, menuName):
        """Creates a new menu in the top bar of Usdview and returns an object
        which can add new items to the menu.
        """

        return self._plugRegistry.getMenu(menuName)

# We load the PluginContainer using libplug so it needs to be a defined Tf.Type.
PluginContainerTfType = Tf.Type.Define(PluginContainer)


class CommandPlugin(object):
    """A Usdview command plugin object. The plugin's `callback` parameter must
    be a callable object which takes a UsdviewApi object as its only parameter.
    """

    def __init__(self, name, callback, usdviewApi):

        self._name = name
        self._callback = callback
        self._usdviewApi = usdviewApi

    @property
    def name(self):
        """Return the command name."""

        return self._name

    def run(self):
        """Run the command's callback function."""

        self._callback(self._usdviewApi)


class PluginMenu(object):
    """Object which adds Usdview command plugins to a QMenu."""

    def __init__(self, qMenu):

        self._qMenu = qMenu
        self._submenus = dict()

    def addItem(self, commandPlugin, shortcut=None):
        """Add a new command plugin to the menu. Optionally, provide a hotkey/
        shortcut.
        """

        action = self._qMenu.addAction(commandPlugin.name,
            lambda: commandPlugin.run())

        if shortcut is not None:
            action.setShortcut(QtGui.QKeySequence(shortcut))

    def getSubmenu(self, menuName):
        """Get a PluginMenu object for the submenu with the given name. If no
        submenu with the given name exists, it is created.
        """

        if menuName in self._submenus:
            return self._submenus[menuName]
        else:
            subQMenu = self._qMenu.addMenu(menuName)
            submenu = PluginMenu(subQMenu)
            self._submenus[menuName] = submenu
            return submenu

    def addSeparator(self):
        """Add a separator to the menu."""

        self._qMenu.addSeparator()


class PluginRegistry(object):
    """Manages all plugins loaded by Usdview."""

    def __init__(self, usdviewApi, mainWindow):

        self._usdviewApi = usdviewApi
        self._mainWindow = mainWindow

        self._commandPlugins = set()

        self._menus = dict()

    def registerCommandPlugin(self, name, callback):
        """Creates, registers, and returns a new command plugin. The plugin's
        `callback` parameter must be a callable object which takes a UsdviewApi
        object as its only parameter.
        """

        plugin = CommandPlugin(name, callback, self._usdviewApi)
        self._commandPlugins.add(plugin)
        return plugin

    def getMenu(self, menuName):
        """Get a PluginMenu object for the menu with the given name. If no menu
        with the given name exists, it is created.
        """

        if menuName in self._menus:
            return self._menus[menuName]
        else:
            qMenu = self._mainWindow.menuBar().addMenu(menuName)
            menu = PluginMenu(qMenu)
            self._menus[menuName] = menu
            return menu


def loadPlugins(usdviewApi, mainWindow):
    """Find and load all Usdview plugins."""

    # Find all the defined container types using libplug.
    containerTypes = Plug.Registry.GetAllDerivedTypes(
        PluginContainerTfType)

    # Get all the libplug plugins for Usdview.
    plugins = dict()
    for containerType in containerTypes:
        plugin = Plug.Registry().GetPluginForType(containerType)
        pluginContainers = plugins.setdefault(plugin, [])
        pluginContainers.append(containerType)

    # Create the Usdview plugin registry.
    registry = PluginRegistry(usdviewApi, mainWindow)

    # Load all discovered containers and let them register their plugins and
    # configure the UI.
    allContainers = []
    for plugin, containerTypes in plugins.items():
        plugin.Load()
        for containerType in containerTypes:
            pluginContainer = containerType.pythonClass(usdviewApi, registry)
            pluginContainer.registerPlugins()
            pluginContainer.configureView()
            allContainers.append(pluginContainer)
