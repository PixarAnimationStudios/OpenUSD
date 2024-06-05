#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

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

    renderers = GetAllPluginArguments()

    argsParser.add_argument('--renderer', '-r', action='store',
        dest='rendererPlugin',
        choices=renderers,
        help=helpText)
