#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function
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
    if path == 'stdout' or path == '-':
        yield sys.stdout
    elif path == 'stderr':
        yield sys.stderr
    else:
        with open(path, *args, **kwargs) as fp:
            yield fp

def _Print(stream, msg):
    print(msg, file=stream)

def main():
    parser = argparse.ArgumentParser(description="""Utility for checking the 
    compliance of a given USD stage or a USDZ package.  Only the first sample
    of any relevant time-sampled attribute is checked, currently.  General
    USD checks are always performed, and more restrictive checks targeted at
    distributable consumer content are also applied when the "--arkit" option
    is specified.""")

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
                        default='stdout', help='The file to which all the failed'
                        ' checks are output. If unspecified, the failed checks '
                        'are output to stdout; if "stderr", terminal coloring '
                        'will be surpressed.')
    parser.add_argument('--noAssetChecks', dest='noAssetChecks', 
                        action='store_true', help='If specified, do NOT perform '
                        'extra checks to help ensure the stage or '
                        'package can be easily and safely referenced into '
                        'aggregate stages.')
    parser.add_argument('--arkit', dest='arkit', action='store_true', 
                        help='Check if the given USD stage is compatible with '
                        'RealityKit\'s implementation of USDZ as of 2023. These assets '
                        'operate under greater constraints that usdz files for '
                        'more general in-house uses, and this option attempts '
                        'to ensure that these constraints are met.')
    parser.add_argument('-d', '--dumpRules', dest='dumpRules', 
                        action='store_true', help='Dump the enumerated set of '
                        'rules being checked for the given set of options.')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='Enable verbose output mode.')
    parser.add_argument('-t', '--strict', dest='strict', action='store_true',
                        help='Return failure code even if only warnings are '
                        'issued, for stricter compliance.')

    args = parser.parse_args()
    inputFile = args.inputFile
    outFile = args.outFile
    
    checker = UsdUtils.ComplianceChecker(arkit=args.arkit, 
                                         skipARKitRootLayerCheck=False, 
                                         rootPackageOnly=args.rootPackageOnly, 
                                         skipVariants=args.skipVariants, 
                                         verbose=args.verbose,
                                         assetLevelChecks=not args.noAssetChecks)

    if not args.dumpRules and not args.inputFile:
        parser.error("Either an inputFile or the --dumpRules option must be"
                     "specified.")

    if args.dumpRules:
        checker.DumpRules()
        # If there's no input file to check, exit after dumping the rules.
        if args.inputFile is None:
            return 0

    checker.CheckCompliance(inputFile)

    warnings = checker.GetWarnings()
    errors = checker.GetErrors()
    failedChecks = checker.GetFailedChecks()
    
    with _Stream(outFile, 'w') as ofp:
        
        for msg in warnings:
            # Add color if we're outputting normally to a terminal.
            if ofp == sys.stdout:
                msg = TermColors.WARN + msg  + TermColors.END
            _Print(ofp, msg)
        
        for msg in errors + failedChecks:
            # Add color if we're outputting normally to a terminal.
            if ofp == sys.stdout:
                msg = TermColors.FAIL + msg  + TermColors.END
            _Print(ofp, msg)

        if (args.strict and len(warnings)) or len(errors) or len(failedChecks):
            print("Failed!")
            return 1

    if len(warnings) > 0:
        print("Success with warnings...")
    else:
        print("Success!")
    return 0

if __name__ == '__main__':
    sys.exit(main())
