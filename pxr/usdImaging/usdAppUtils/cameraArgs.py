#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def AddCmdlineArgs(argsParser, defaultValue=None, altHelpText=''):
    """
    Adds camera-related command line arguments to argsParser.

    The resulting 'camera' argument will be an Sdf.Path. If no value is given
    and defaultValue is not overridden, 'camera' will be a single-element path
    containing the primary camera name.
    """
    from pxr import Sdf
    from pxr import UsdUtils

    if defaultValue is None:
        defaultValue = UsdUtils.GetPrimaryCameraName()

    helpText = altHelpText
    if not helpText:
        helpText = (
            'Which camera to use - may be given as either just the '
            'camera\'s prim name (i.e. just the last element in the prim '
            'path), or as a full prim path. Note that if only the prim name '
            'is used and more than one camera exists with that name, which '
            'one is used will effectively be random (default=%(default)s)')

    # This avoids an Sdf warning if an empty string is given, which someone
    # might do for example with usdview to open the app using the 'Free' camera
    # instead of the primary camera.
    def _ToSdfPath(inputArg):
        if not inputArg:
            return Sdf.Path.emptyPath
        return Sdf.Path(inputArg)

    argsParser.add_argument('--camera', '-cam', action='store',
        type=_ToSdfPath, default=defaultValue, help=helpText)
