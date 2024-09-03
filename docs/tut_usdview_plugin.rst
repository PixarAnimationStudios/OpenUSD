.. include:: rolesAndUtils.rst

.. include:: tut_setup_version_badge.rst   

=========================
Creating a Usdview Plugin
=========================

In this tutorial, we'll create a new Python plugin for :program:`usdview` and 
learn how to use the :program:`usdview` API.

Setting Up a PluginContainer
============================

First, we'll create a new directory to hold our new plugin. We can put this
directory anywhere that our USD build will look for plugins, but it's a good
idea to nest it in a directory that can hold other plugins in case we want to
install more in the future.

.. code-block:: console

  mkdir -p <some path>/usdviewPlugins/tutorialPlugin/

We now want to create a new PluginContainer class in our plugin module's
__init__.py file.

.. code-block:: python
   :caption: tutorialPlugin/__init__.py

   from pxr import Tf
   from pxr.Usdviewq.plugin import PluginContainer
   
   
   def printMessage(usdviewApi):
       print("Hello, World!")
   
   
   class TutorialPluginContainer(PluginContainer):
   
       def registerPlugins(self, plugRegistry, usdviewApi):
   
           self._printMessage = plugRegistry.registerCommandPlugin(
               "TutorialPluginContainer.printMessage",
               "Print Message",
               printMessage)
   
       def configureView(self, plugRegistry, plugUIBuilder):
   
           tutMenu = plugUIBuilder.findOrCreateMenu("Tutorial")
           tutMenu.addItem(self._printMessage)
   
   Tf.Type.Define(TutorialPluginContainer)

PluginContainers are just objects that know how to register new command plugins
and add them to usdview's UI. This is done through 2 methods:

When a :python:`PluginContainer` is loaded, usdview's plugin system first 
calls :python:`registerPlugins()` which gives the container a chance to add 
its command plugins to the plugin registry. Each command plugin needs an 
identifier string, display name, and callback function. The identifier must be 
globally unique, so it is good practice to prepend the name of the plugin 
container. All command plugin callbacks take a :python:`usdviewApi` object as 
their only parameter (we'll learn how to use the API later). In our example, 
we registered a new plugin that prints "Hello, World!" when invoked.

After a :python:`PluginContainer` has registered all of its command plugins, 
it is given the chance to expose them to usdview's UI in the 
:python:`configureView()` method. Currently, plugins can only create simple 
menus in usdview's menu bar as well as open new Qt windows. Our example 
creates a new menu named "Tutorial" and adds our "printMessage" command plugin 
to the menu.

Since plugins are loaded through Pixar's `libplug
<api/plug_page_front.html>`_ library, we
also need to define our :python:`PluginContainer` as a new Tf.Type. This just 
lets libplug find the container later. We also need to create a new 
plugInfo.json file in our plugin directory, so we'll do that now.

.. code-block:: json
   :caption: plugInfo.json

   {
       "Plugins": [
           {
               "Type": "python",
               "Name": "tutorialPlugin",
               "Info": {
                   "Types": {
                       "tutorialPlugin.TutorialPluginContainer": {
                           "bases": ["pxr.Usdviewq.plugin.PluginContainer"],
                           "displayName": "Usdview Tutorial Plugin"
                       }
                   }
               }
           }
       ]
   }

When making a plugin container, all we need to change from the above example is
the "Name" field, to match our plugin's Python module name, change the 
"tutorialPlugin.TutorialPluginContainer" type to match our 
:python:`PluginContainer` type name, and update the "displayName."

Lastly, we need to configure the environment. libplug loads Python plugins by
importing the module directly, so we need to make sure our plugins directory 
(if trying the sendMail.py example in 
:filename:`extras/usd/examples/usdviewPlugins/` it would be that directory, 
NOT :filename:`tutorialPlugin/`) is listed in our PYTHONPATH environment 
variable. If we want libplug to load our plugin, we also need to add the path 
to the plugin directory (this time :filename:`tutorialPlugin/` in our example) 
to the PXR_PLUGINPATH_NAME environment variable.

At this point, if we open :program:`usdview` we should see a new "Tutorial" 
menu. If we open this menu and select "Print Message," we should see "Hello, 
World!" printed to the console.

Congratulations! We have just created a new :program:`usdview` plugin!

Using the usdview API
=====================

Now that we can create command plugins, we can start interacting with 
:program:`usdview` using the :python:`usdviewApi` object. An overview of API 
features is given below below. For a full listing of all API features, open 
the Interpreter window in :program:`usdview` 
(:menuselection:`Window --> Interpreter`), and type :python:`help(usdviewApi)`.

    * :python:`usdviewApi.dataModel`- a full representation of usdview's 
      state. The majority of the data and functionality available to plugins 
      is available through the data model.

        * :python:`stage` - The current Usd.Stage object.

        * :python:`currentFrame` - usdview's current frame.

        * :python:`viewSettings` - A collection of settings which only affect 
          the viewport. Most of these settings are normally controlled using 
          usdview's 'Display' menu. Some examples are listed below.

            * :python:`complexity` - The scene's subdivision complexity.

            * :python:`freeCamera` - The camera object used when 
              :program:`usdview` is not viewing through a camera prim. Plugins 
              can modify this camera to change the view.

            * :python:`renderMode` - The mode used for rendering models 
              (smooth-shaded, flat-shaded, wireframe, etc.).

        * :python:`selection` - The current state of prim and property 
          selections.

            * Common prim selection methods: :python:`getFocusPrim()`, 
              :python:`getPrims()`, :python:`setPrim(prim)`, 
              :python:`addPrim(prim)`, :python:`clearPrims()`

            * Common property selection methods: :python:`getFocusProp()`, 
              :python:`getProps()`, :python:`setProp(prop)`, 
              :python:`addProp(prop)`, :python:`clearProps()`

    * :python:`usdviewApi.qMainWindow` - usdview's Qt MainWindow object. It 
      can be used as a parent for other Qt windows and dialogs, but it should 
      not be used for any other purpose.

    * :python:`usdviewApi.PrintStatus(msg)` - Prints a status message at the 
      bottom of the :program:`usdview` window.

    * :python:`GrabViewportShot()`/:python:`GrabWindowShot()` - Captures a 
      screenshot of the viewport or the entire main window and returns it as a 
      QImage.

Deferring Imports
=================

:program:`usdview` is designed to be quick to launch, so to be a good 
:program:`usdview` citizen we should make sure our plugin loads as quickly as 
possible. Sometimes, importing other Python modules takes a noticeable amount 
of time, so it is a good idea to import them lazily when our command plugin is 
called for the first time.

The easiest way to do this is by putting our plugin logic into a separate 
Python file and using the :python:`deferredImport(moduleName)` method 
available on PluginContainers. Let's fix the above example to use this method.

First, we'll put our printMessage function into a new Python file called
printer.py. This function doesn't require any heavy imports, so we'll just 
print a message when the file is imported so we know it was deferred properly.

.. code-block:: python
   :caption: tutorialPlugin/printer.py

   print("Imported printer!")
   
   
   def printMessage(usdviewApi):
       print("Hello, World!")

Then, we'll import the module the normal way (without deferring yet) in
__init__.py and make sure to call the printMessage function off this new 
module.

.. code-block:: python
   :caption: tutorialPlugin/__init__.py - Normal Import

   from pxr import Tf
   from pxr.Usdviewq.plugin import PluginContainer
   
   import printer
   
   
   class TutorialPluginContainer(PluginContainer):
   
       def registerPlugins(self, plugRegistry, usdviewApi):
   
           self._printMessage = plugRegistry.registerCommandPlugin(
               "TutorialPluginContainer.printMessage",
               "Print Message",
               printer.printMessage)
   
       def configureView(self, plugRegistry, plugUIBuilder):
   
           tutMenu = plugUIBuilder.findOrCreateMenu("Tutorial")
           tutMenu.addItem(self._printMessage)
   
   Tf.Type.Define(TutorialPluginContainer)

If we run :program:`usdview` now, we'll immediately see the message "Imported 
printer!" appear in the console. Now, we'll do a deferred import.

.. code-block:: python
   :caption: tutorialPlugin/__init__.py - Deferred Import

   from pxr import Tf
   from pxr.Usdviewq.plugin import PluginContainer
   
   
   class TutorialPluginContainer(PluginContainer):
   
       def registerPlugins(self, plugRegistry, usdviewApi):
   
           printer = self.deferredImport(".printer")
           self._printMessage = plugRegistry.registerCommandPlugin(
               "TutorialPluginContainer.printMessage",
               "Print Message",
               printer.printMessage)
   
       def configureView(self, plugRegistry, plugUIBuilder):
   
           tutMenu = plugUIBuilder.findOrCreateMenu("Tutorial")
           tutMenu.addItem(self._printMessage)
   
   Tf.Type.Define(TutorialPluginContainer)

All we did was remove printer from our imports and add line 9. The
deferredImport method just returns a fake module object, and pretends to know
about any function we access on it. The first time one of its functions is
called, it actually imports the target module and calls its function instead.

.. note:: :python:`deferredImport` doesn't know anything about the target 
          module until it is imported, so it assumes any object we reference 
          is a function in the module. If it refers to a function that doesn't 
          exist on the target module, we'll get an ImportError.

If we run :program:`usdview` again, we won't see the "Imported printer!" 
message until we invoke :python:`printMessage`. The module is only imported 
once, so invoking :python:`printMessage` multiple times will only print the 
message the first time.

SendMail Example Plugin
=======================

An example plugin file is provided with the USD distribution at 
:filename:`USD/extras/usd/examples/usdviewPlugins/sendMail.py`. We can add this
plugin to our :python:`PluginContainer` and specify :python:`sendMail.SendMail`
as the command callback function.

When SendMail is invoked, a dialog opens which prompts the user to send an 
email that contains a screenshot of :program:`usdview`. The user can modify 
the recipient, subject, and body of the email and select whether to send a 
screenshot of the entire main :program:`usdview` window or just the render 
viewport.

If we inspect :filename:`sendMail.py`, we can see it call 
:python:`usdviewApi.GrabWindowShot()` and 
:python:`usdviewApi.GrabViewportShot()` to capture both types of screenshot. 
We can also see an example of creating a dialog parented to the 
:program:`usdview` main window using :python:`usdviewApi.qMainWindow`. The 
plugin also includes several pieces of data from the API to the email body.

Organizing usdview Plugins in a Production Environment
======================================================

Although the :python:`PluginContainer` system allows for any number of plugin 
modules to be discovered and executed, its design is meant to make it easier 
for "non-build experts" to add new :program:`usdview` plugins. Although in 
this simple tutorial our :python:`registerPlugins()` method registered only a 
single command, it is capable of registering an arbitrary number of commands, 
and :python:`configureView()` can create and configure any number of menus. 
**Putting all plugins in a single module**, which is what we do at Pixar, has 
two advantages:

    #. Once that module is set up by a maintainer, users desiring to add new
       plugins do not need to know about or modify any plugInfo.json files

    #. It is much easier to organize all commands into a coherent, well-ordered
       set of menus when that setup happens in a single place.
