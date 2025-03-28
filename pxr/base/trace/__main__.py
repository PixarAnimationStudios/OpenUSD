#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
