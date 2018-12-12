#!/pxrpythonsubst
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
import argparse
import itertools
import sys

from contextlib import contextmanager

from pxr import Ar, Sdf, Usd, UsdGeom, UsdShade, UsdUtils

class TermColors:
    WARN = '\033[93m'
    FAIL = '\033[91m'
    END = '\033[0m'

@contextmanager
def _Stream(path, *args, **kwargs):
    if path == '-':
        yield sys.stdout
    else:
        with open(path, *args, **kwargs) as fp:
            yield fp

def _Print(stream, msg):
    print >>stream, msg

def _Err(msg):
    sys.stderr.write(TermColors.FAIL + msg + TermColors.END + '\n')

def _Warn(msg):
    sys.stderr.write(TermColors.WARN + msg + TermColors.END + '\n')

def main():
    parser = argparse.ArgumentParser(description='Utility for checking the '
        'compliance of a given USD stage or a USDZ package.')

    parser.add_argument('inputFile', type=str, nargs='?', 
                        help='Name of the input file to inspect.')
    parser.add_argument('-s', '--skipVariants', dest='skipVariants',
                        action='store_true', help='If specified, only the prims'
                        ' that are present in the default (i.e. selected) '
                        'variants are checked. When this option is not '
                        'specified, prims in all possible combinations of '
                        'variant selections are checked.')
    parser.add_argument('-p', '--rootPackageOnly', dest='rootPackageOnly', 
                        action="store_true", help='Check only the specified'
                        'package. Nested packages, dependencies and their '
                        'contents are not validated.')
    parser.add_argument('-o', '--out', dest='outFile', type=str, nargs='?',
                        default='-', help='The file to which all the failed '
                        'checks are output. If unspecified, the failed checks '
                        'are output to stdout.')
    parser.add_argument('--arkit', dest='arkit', action='store_true', 
                        help='Check if the given USD stage is compatible with '
                        'ARKit\'s initial implementation of usdz. These assets '
                        'operate under greater constraints that usdz files for '
                        'more general in-house uses, and this option attempts '
                        'to ensure that these constraints are met.')
    parser.add_argument('-d', '--dumpRules', dest='dumpRules', 
                        action='store_true', help='Dump the enumerated set of '
                        'rules being checked for the given set of options.')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='Enable verbose output mode.')

    args = parser.parse_args()
    inputFile = args.inputFile
    outFile = args.outFile
    
    checker = UsdUtils.ComplianceChecker(arkit=args.arkit, 
            skipARKitRootLayerCheck=False, rootPackageOnly=args.rootPackageOnly, 
            skipVariants=args.skipVariants, verbose=args.verbose)

    if not args.dumpRules and not args.inputFile:
        parser.error("Either an inputFile or the --dumpRules option must be"
                     "specified.")

    if args.dumpRules:
        checker.DumpRules()
        # If there's no input file to check, exit after dumping the rules.
        if args.inputFile is None:
            return 0

    checker.CheckCompliance(inputFile)

    errors = checker.GetErrors()
    failedChecks = checker.GetFailedChecks()
    
    if len(errors)> 0 or len(failedChecks) > 0:
        with _Stream(outFile, 'w') as ofp:
            for msg in errors + failedChecks:
                # Add color if we're outputting to a terminal.
                if outFile == '-':
                    msg = TermColors.FAIL + msg  + TermColors.END
                _Print(ofp, msg)
        print "Failed!"
        return 1

    print "Success!"
    return 0

if __name__ == '__main__':
    sys.exit(main())
