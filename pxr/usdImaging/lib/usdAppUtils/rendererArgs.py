#
# Copyright 2019 Pixar
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

class RendererPlugins(object):
    """
    An enum-like container of the available Hydra renderer plugins.
    """

    class _RendererPlugin(object):
        """
        Class which represents a Hydra renderer plugin. Each one has a plugin
        identifier and a display name.
        """

        def __init__(self, pluginId, displayName):
            self._pluginId = pluginId
            self._displayName = displayName

        def __repr__(self):
            return self.displayName

        @property
        def id(self):
            return self._pluginId

        @property
        def displayName(self):
            return self._displayName

    @classmethod
    def allPlugins(cls):
        """
        Get a tuple of all available renderer plugins.
        """
        if not hasattr(cls, '_allPlugins'):
            from pxr import UsdImagingGL
            cls._allPlugins = tuple(cls._RendererPlugin(pluginId,
                    UsdImagingGL.Engine.GetRendererDisplayName(pluginId))
                for pluginId in UsdImagingGL.Engine.GetRendererPlugins())

        return cls._allPlugins

    @classmethod
    def fromId(cls, pluginId):
        """
        Get a renderer plugin from its identifier.
        """
        matches = [plugin for plugin in cls.allPlugins() if plugin.id == pluginId]
        if len(matches) == 0:
            raise ValueError("No renderer plugin with id '{}'".format(pluginId))
        return matches[0]

    @classmethod
    def fromDisplayName(cls, displayName):
        """
        Get a renderer plugin from its display name.
        """
        matches = [plugin for plugin in cls.allPlugins() if plugin.displayName == displayName]
        if len(matches) == 0:
            raise ValueError("No renderer plugin with display name '{}'".format(displayName))
        return matches[0]

def AddCmdlineArgs(argsParser, altHelpText=''):
    """
    Adds Hydra renderer-related command line arguments to argsParser.

    The resulting 'rendererPlugin' argument will be a _RendererPlugin instance
    representing one of the available Hydra renderer plugins.
    """
    from pxr import UsdImagingGL

    helpText = altHelpText
    if not helpText:
        helpText = (
            'Hydra renderer plugin to use when generating images')

    argsParser.add_argument('--renderer', '-r', action='store',
        type=RendererPlugins.fromDisplayName,
        dest='rendererPlugin',
        choices=[p for p in RendererPlugins.allPlugins()],
        help=helpText)
