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

# Execute a script with tracing enabled.

# Usage: python -m pxr.Trace [-o OUTPUT] [-c "command" | FILE]

from __future__ import print_function
import argparse, sys

parser = argparse.ArgumentParser(description='Trace script execution.')
parser.add_argument('-o', dest='output', metavar='OUTPUT',
                    type=str, default=None,
                    help='trace output; defaults to stdout')
parser.add_argument('-c', dest='cmd',
                    type=str,
                    help='trace <cmd> as a Python script')
parser.add_argument('file', metavar="FILE",
                    nargs='?',
                    default=None,
                    help='script to trace')

args = parser.parse_args()

if not args.cmd and not args.file:
    print("Must specify a command or script to trace", file=sys.stderr)
    sys.exit(1)

if args.cmd and args.file:
    print("Only one of -c or FILE may be specified", file=sys.stderr)
    sys.exit(1)

from pxr import Trace

env = {}

# Create a Main function that is traced so we always capture a useful
# top-level scope.
if args.file:
    @Trace.TraceFunction
    def Main():
        exec(compile(open(args.file).read(), args.file, 'exec'), env)
else:
    @Trace.TraceFunction
    def Main():
        exec(args.cmd, env)

try:
    Trace.Collector().enabled = True
    Main()
finally:
    Trace.Collector().enabled = False

    if args.output is not None:
        Trace.Reporter.globalReporter.Report(args.output)
    else:
        Trace.Reporter.globalReporter.Report()
