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

def _GetFloatStringPrecision(floatString):
    """
    Gets the floating point precision specified by floatString.

    floatString can either contain an actual float in string form, or it can be
    a frame placeholder. We simply split the string on the dot (.) and return
    the length of the part after the dot, if any.

    If there is no dot in the string, a precision of zero is assumed.
    """
    floatPrecision = 0

    if not floatString:
        return floatPrecision

    floatStringParts = floatString.split('.')
    if len(floatStringParts) > 1:
        floatPrecision = len(floatStringParts[1])

    return floatPrecision

class FrameSpecIterator(object):
    """
    A simple iterator object that handles splitting multiple comma-separated
    FrameSpecs into their equivalent UsdUtils.TimeCodeRanges, and then yields
    all of the time codes in all of those ranges sequentially when iterated.
    
    This object also stores the minimum floating point precision required to
    disambiguate any neighboring time codes in the FrameSpecs given. This can
    be used to validate that the frame placeholder in a frame format string has
    enough precision to uniquely identify every frame without collisions.
    """
    from pxr import UsdUtils

    FRAMESPEC_SEPARATOR = ','

    def __init__(self, frameSpec):
        from pxr import UsdUtils

        # Assume all frames are integral until we find a frame spec with a
        # non-integral stride.
        self._minFloatPrecision = 0

        self._timeCodeRanges = []
        subFrameSpecs = frameSpec.split(self.FRAMESPEC_SEPARATOR)
        for subFrameSpec in subFrameSpecs:
            timeCodeRange = UsdUtils.TimeCodeRange.CreateFromFrameSpec(
                subFrameSpec)
            self._timeCodeRanges.append(timeCodeRange)

            specParts = subFrameSpec.split(
                UsdUtils.TimeCodeRange.Tokens.StrideSeparator)
            if len(specParts) == 2:
                stride = specParts[1]
                stridePrecision = _GetFloatStringPrecision(stride)
                self._minFloatPrecision = max(self._minFloatPrecision,
                    stridePrecision)

    def __iter__(self):
        for timeCodeRange in self._timeCodeRanges:
            for timeCode in timeCodeRange:
                yield timeCode

    @property
    def minFloatPrecision(self):
        return self._minFloatPrecision


def AddCmdlineArgs(argsParser, altDefaultTimeHelpText='', altFramesHelpText=''):
    """
    Adds frame-related command line arguments to argsParser.

    The resulting 'frames' argument will be an iterable of UsdTimeCodes.

    If no command-line arguments are given, 'frames' will be a list containing
    only Usd.TimeCode.EarliestTime(). If '--defaultTime' is given, 'frames'
    will be a list containing only Usd.TimeCode.Default(). Otherwise,
    '--frames' must be given a FrameSpec (or a comma-separated list of
    multiple FrameSpecs), and 'frames' will be a FrameSpecIterator which when
    iterated will yield the time codes specified by the FrameSpec(s).
    """
    timeGroup = argsParser.add_mutually_exclusive_group()

    helpText = altDefaultTimeHelpText
    if not helpText:
        helpText = (
            'explicitly operate at the Default time code (the default '
            'behavior is to operate at the Earliest time code)')
    timeGroup.add_argument('--defaultTime', '-d', action='store_true',
        dest='defaultTime', help=helpText)

    helpText = altFramesHelpText
    if not helpText:
        helpText = (
            'specify FrameSpec(s) of the time codes to operate on - A '
            'FrameSpec consists of up to three floating point values for the '
            'start time code, end time code, and stride of a time code range. '
            'A single time code can be specified, or a start and end time '
            'code can be specified separated by a colon (:). When a start '
            'and end time code are specified, the stride may optionally be '
            'specified as well, separating it from the start and end time '
            'codes with (x). Multiple FrameSpecs can be combined as a '
            'comma-separated list. The following are examples of valid '
            'FrameSpecs: 123 - 101:105 - 105:101 - 101:109x2 - 101:110x2 - '
            '101:104x0.5')
    timeGroup.add_argument('--frames', '-f', action='store', type=str,
        dest='frames', metavar='FRAMESPEC[,FRAMESPEC...]',
        help=helpText)

def GetFramePlaceholder(frameFormat):
    """
    Gets the frame placeholder in a frame format string.

    This function expects the input frameFormat string to contain exactly one
    frame placeholder. The placeholder must be composed of exactly one or two
    groups of one or more hashes ('#'), and if there are two, they must be
    separated by a dot ('.').

    If no such placeholder exists in the frame format string, None is returned.
    """
    if not frameFormat:
        return None

    import re

    PLACEHOLDER_PATTERN = r'^[^#]*(?P<placeholder>#+(\.#+)?)[^#]*$'

    matches = re.search(PLACEHOLDER_PATTERN, frameFormat)
    if not matches:
        return None

    placeholder = matches.group(1)

    return placeholder

def ConvertFramePlaceholderToFloatSpec(frameFormat):
    """
    Converts the frame placeholder in a frame format string to a Python
    {}-style float specifier for use with string.format().

    This function expects the input frameFormat string to contain exactly one
    frame placeholder. The placeholder must be composed of exactly one or two
    groups of one or more hashes ('#'), and if there are two, they must be
    separated by a dot ('.').

    The hashes after the dot indicate the floating point precision to use in
    the frame numbers inserted into the frame format string. If there is only
    a single group of hashes, the precision is zero and the inserted frame
    numbers will be integer values.

    The overall width of the frame placeholder specifies the minimum width to
    use when inserting frame numbers into the frame format string. Formatted
    frame numbers smaller than the minimum width will be zero-padded on the
    left until they reach the minimum width.

    If the input frame format string does not contain exactly one frame
    placeholder, this function will return None, indicating that this frame
    format string cannot be used when operating with a frame range.
    """
    placeholder = GetFramePlaceholder(frameFormat)
    if not placeholder:
        return None

    # Frame numbers are zero-padded up to the field width.
    specFill = 0

    # The full width of the placeholder determines the minimum field width.
    specWidth = len(placeholder)

    # The hashes after the dot, if any, determine the precision. If there are
    # none, integer frame numbers are used.
    specPrecision = 0
    parts = placeholder.split('.')
    if len(parts) > 1:
        specPrecision = len(parts[1])

    floatSpec = ('{frame:' +
        '{fill}{width}.{precision}f'.format(fill=specFill,
            width=specWidth, precision=specPrecision) +
        '}')

    return frameFormat.replace(placeholder, floatSpec)

def ValidateCmdlineArgs(argsParser, args, frameFormatArgName=None):
    """
    Validates the frame-related arguments in args parsed by argsParser.

    This populates 'frames' with the appropriate iterable based on the
    command-line arguments given, so it should be called after parse_args() is
    called on argsParser.

    When working with frame ranges, particularly when writing out images for
    each frame, it is common to also have command-line arguments such as an
    output image path for specifying where those images should be written. The
    value given to this argument should include a frame placeholder so that it
    can have the appropriate time code inserted. If the application has such an
    argument, its name can be specified using frameFormatArgName. That arg will
    be checked to ensure that it has a frame placeholder and it will be given
    a value with that placeholder replaced with a Python format specifier so
    that the value is ready to use with the str.format(frame=<timeCode>)
    method. If a frame range is not provided as an argument, then it is an
    error to include a frame placeholder in the frame format string.
    """
    from pxr import Usd

    framePlaceholder = None
    frameFormat = None
    if frameFormatArgName is not None:
        frameFormat = getattr(args, frameFormatArgName)
        framePlaceholder = GetFramePlaceholder(frameFormat)
        frameFormat = ConvertFramePlaceholderToFloatSpec(frameFormat)

    if args.frames:
        args.frames = FrameSpecIterator(args.frames)

        if frameFormatArgName is not None:
            if not frameFormat:
                argsParser.error('%s must contain exactly one frame number '
                    'placeholder of the form "###"" or "###.###". Note that '
                    'the number of hash marks is variable in each group.' %
                    frameFormatArgName)

            placeholderPrecision = _GetFloatStringPrecision(framePlaceholder)

            if placeholderPrecision < args.frames.minFloatPrecision:
                argsParser.error('The given FrameSpecs require a minimum '
                    'floating point precision of %d, but the frame '
                    'placeholder in %s only specified a precision of %d (%s). '
                    'The precision of the frame placeholder must be equal to '
                    'or greater than %d.' % (args.frames.minFloatPrecision,
                        frameFormatArgName, placeholderPrecision,
                        framePlaceholder,args.frames.minFloatPrecision))

            setattr(args, frameFormatArgName, frameFormat)
    else:
        if frameFormat:
            argsParser.error('%s cannot contain a frame number placeholder '
                'when not operating on a frame range.' % frameFormatArgName)

        if args.defaultTime:
            args.frames = [Usd.TimeCode.Default()]
        else:
            args.frames = [Usd.TimeCode.EarliestTime()]

    return args
