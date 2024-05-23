#!/pxrpythonsubst
#
# Copyright 2024 Pixar
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
"""
This script generates a toolset.help file that is used to create the 
documentation for the Toolset page (https://openusd.org/release/toolset.html)
The script creates toolset.help with the cmdline "help" output from the 
below list of tools. 
"""

import sys
import os
from argparse import ArgumentParser
import subprocess
import shutil

# Name of script, e.g. "genToolsetDoc"
PROGRAM_NAME = os.path.splitext(os.path.basename(sys.argv[0]))[0]

# Default name of output file
OUTPUT_FILE = "toolset.help"

# List of tools to run and the "help" cmdline arg for each tool
progAndArgList = [("usdedit", "-h"),
("usdcat", "-h"),
("usddiff", "-h"),
("usdview","-h"),
("usdrecord","-h"),
("usdresolve","-h"),
("usdtree","-h"),
("usdzip","-h"),
("usdchecker","-h"),
("usdfixbrokenpixarschemas", "-h"),
("usdstitch", "-h"),
("usdstitchclips", "-h"),
("usddumpcrate", "-h"),
("sdfdump", "-h"),
("sdffilter", "-h")
]

#------------------------------------------------------------------------------#
# Helper functions                                                             #
#------------------------------------------------------------------------------#

def _GetArgs():
    """
    Get the cmdline args
    """
    usage = """Generate toolset.help used in the toolset.rst page. Assumes tools
 are in the current PATH."""
    parser = ArgumentParser(description=usage)
    parser.add_argument('outputPath',
        nargs='?',
        type=str,
        default="./" + OUTPUT_FILE,
        help='The path and filename for the generated file. '
        '[Default: %(default)s]')
    resultArgs = parser.parse_args()
    return resultArgs


def _RunCommandAndCaptureStdout(commandName, arg):
    """
    Run the given cmdline command and capture and return the resulting stdout
    as a string
    """
    result = subprocess.run([commandName, arg], stdout=subprocess.PIPE)
    # Convert from bytes to UTF-8
    resultStr = result.stdout.decode('utf-8')
    return resultStr


#------------------------------------------------------------------------------#
# main                                                                         #
#------------------------------------------------------------------------------#

if __name__ == '__main__':

    # Parse Command-line
    args = _GetArgs()

    # Check if outputPath is just a path, and append default filename if so
    if os.path.isdir(args.outputPath):
        # Append default filename to path
        outputFile = os.path.join(args.outputPath, OUTPUT_FILE)
    else:
        outputFile = args.outputPath

    try:
        with open(outputFile, 'w') as file:
            # Run our list of tools with "-h" and capture stdout and append to file
            # (with markers so that output can be included in a codeblock in Sphinx RST)
            # Note this assumes the desired version of the built tools are in the
            # current process path.
            for cmdAndArg in progAndArgList:
                cmdStr = _RunCommandAndCaptureStdout(cmdAndArg[0], cmdAndArg[1])
                # To guard against some commands which output the entire path in 
                # their help output, we use shutil to get the binary path and
                # filter out the path portion in cmdStr if found.
                cmdPath = shutil.which(cmdAndArg[0])
                cmdStr = cmdStr.replace(cmdPath, os.path.basename(cmdPath))
                # Use markers in the file to separate output from different commands
                # needed by Sphinx literalinclude directives in toolset.rst
                workStr = "\n==== " + cmdAndArg[0] + " start ====\n"
                file.write(workStr)
                file.write(cmdStr)
                workStr = "\n==== " + cmdAndArg[0] + " end ====\n"
                file.write(workStr)
    except IOError as openException:
        print("Failed to write to " + outputFile + ": " + str(openException), file=sys.stderr)
        sys.exit(1)
