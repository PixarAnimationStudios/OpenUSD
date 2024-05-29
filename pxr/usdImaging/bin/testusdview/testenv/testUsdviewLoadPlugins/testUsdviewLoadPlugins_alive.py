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
extraPluginDir = os.path.join(os.getcwd(), "plugins/extraPlugin/")
Plug.Registry().RegisterPlugins(extraPluginDir)


# Add extraPlugin to the libplug search path, then verify that it was loaded by
# checking for one of its commands in the registry.
def testUsdviewInputFunction(appController):
    registry = appController._plugRegistry

    assert registry is not None

    assert registry.getCommandPlugin("ExtraContainer.extraCommand1") is not None
