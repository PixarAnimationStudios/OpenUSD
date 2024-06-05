#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import os, sys

from pxr import Plug


# Append to Python's search path a relative path to the plugins directory so
# libplug can load plugin modules from that directory.
sys.path.append("./plugins/")


# Register a new plugInfo.json. Unfortunately, plugins are already loaded by the
# time we get to testUsdviewInputFunction, so this has to be placed here.
extraPluginDir = os.path.join(os.getcwd(), "plugins/extraPluginMissingContainer/")
Plug.Registry().RegisterPlugins(extraPluginDir)

# We register a second plugInfo.json in this test so the registry is created.
# That way we can verify that the missing container did not register any
# command plugins.
extraPluginDir = os.path.join(os.getcwd(), "plugins/extraPlugin/")
Plug.Registry().RegisterPlugins(extraPluginDir)


# We shouldn't be able to find extraPluginMissingContainer's container, so we
# shouldn't be able to find its command plugins.
def testUsdviewInputFunction(appController):
    registry = appController._plugRegistry

    assert registry is not None

    assert registry.getCommandPlugin("ExtraContainer.extraCommand1") is not None
    assert registry.getCommandPlugin(
        "ExtraContainerMissingContainer.extraCommand1") is None
