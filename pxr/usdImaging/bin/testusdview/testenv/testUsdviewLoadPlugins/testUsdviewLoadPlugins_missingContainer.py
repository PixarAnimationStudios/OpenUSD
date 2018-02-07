#!/pxrpythonsubst
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
