#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import argparse

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
        if argumentString == UsdImagingGL.Engine.GetRendererDisplayName(p) or argumentString == p:
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
            'Hydra renderer plugin to use when generating images. "GL" and '
            '"Storm" currently alias to the same renderer, Storm.')

    class RendererChoices(list):
        def __contains__(self, other):
            return super().__contains__(other) or UsdImagingGL.Engine.GetRendererPlugins().__contains__(other)

    renderers = RendererChoices(GetAllPluginArguments())
    # "GL" is still (unfortunately) the offical display name for Storm, but 
    # we've hacked UsdImagingGLEngine::GetRendererDisplayName to instead 
    # return "Storm" for the HdStormRendererPlugin. We still wish to support 
    # "GL" as an argument here, however. Both "GL" and "Storm" refer to the 
    # Storm renderer.
    renderers.append('GL')

    class ResolveStormAlias(argparse.Action):
        def __call__(self, parser, namespace, value, option_string):
            if value == 'Storm':
                setattr(namespace, self.dest, 'GL')
            else:
                setattr(namespace, self.dest, value)

    argsParser.add_argument('--renderer', '-r',
        dest='rendererPlugin',
        choices=renderers,
        help=helpText,
        action=ResolveStormAlias)
    
