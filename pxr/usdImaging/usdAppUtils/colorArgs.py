#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def AddCmdlineArgs(argsParser, defaultValue='sRGB', altHelpText=''):
    """
    Adds color-related command line arguments to argsParser.

    The resulting 'colorCorrectionMode' argument will be a Python string.
    """
    helpText = altHelpText
    if not helpText:
        helpText = (
            'the color correction mode to use (default=%(default)s)')

    argsParser.add_argument('--colorCorrectionMode', '-color', action='store',
        type=str, default=defaultValue,
        choices=['disabled', 'sRGB', 'openColorIO'], help=helpText)
