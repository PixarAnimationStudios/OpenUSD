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

def AddCmdlineArgs(argsParser, defaultValue='color', altHelpText=''):
    """
    Adds output aov command line arguments to argsParser.

    The resulting 'aov' argument will be a Python string.
    """
    helpText = altHelpText
    if not helpText:
        helpText = (
            'the aov mode to use (default=%(default)s)')

    aovChoices = ['color', 'depth', 'primId']
    argsParser.add_argument('--aov', action='store',
                            type=str, choices=aovChoices, default=defaultValue, help=helpText)
