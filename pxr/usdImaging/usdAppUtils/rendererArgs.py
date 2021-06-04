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

HYDRA_DISABLED_OPTION_STRING = "HydraDisabled"

def GetAllPluginArguments():
    """
    Returns argument strings for all the renderer plugins available.
    """

    from pxr import UsdImagingGL
    return [ UsdImagingGL.Engine.GetRendererDisplayName(pluginId) for
                pluginId in UsdImagingGL.Engine.GetRendererPlugins() ]


def GetPluginIdFromArgument(argumentString):
    """
    Returns plugin id, if found, for the passed in argument string.

    Valid argument strings are returned by GetAllPluginArguments().
    """

    from pxr import UsdImagingGL
    for p in UsdImagingGL.Engine.GetRendererPlugins():
        if argumentString == UsdImagingGL.Engine.GetRendererDisplayName(p):
            return p
    return None


def AddCmdlineArgs(argsParser, altHelpText='', allowHydraDisabled=False):
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

    renderers = GetAllPluginArguments()
    if allowHydraDisabled:
        renderers.append(HYDRA_DISABLED_OPTION_STRING)

    argsParser.add_argument('--renderer', '-r', action='store',
        dest='rendererPlugin',
        choices=renderers,
        help=helpText)
