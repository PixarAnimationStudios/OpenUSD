#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import print_function
import argparse, sys, os
from pxr import Sdf, Usd

def Err(msg):
    print('Error: ' + msg, file=sys.stderr)

def PrintReport(fname, info, summaryOnly):
    print('@%s@' % fname, 'file version', info.GetFileVersion())
    ss = info.GetSummaryStats()
    print ('  %s specs, %s paths, %s tokens, %s strings, '
           '%s fields, %s field sets' % 
           (ss.numSpecs, ss.numUniquePaths, ss.numUniqueTokens,
            ss.numUniqueStrings, ss.numUniqueFields, ss.numUniqueFieldSets))
    if summaryOnly:
        return
    print('  Structural Sections:')
    for sec in info.GetSections():
        print('    %16s %16d bytes at offset 0x%X' % (
            sec.name, sec.size, sec.start))
    print()

def main():
    parser = argparse.ArgumentParser(
        prog=os.path.basename(sys.argv[0]), 
        description='Write information about a usd crate (usdc) file to stdout')

    parser.add_argument('inputFiles', nargs='+')
    parser.add_argument('-s', '--summary', action='store_true', 
                        help='report only a short summary')

    args = parser.parse_args()

    print('Usd crate software version', Usd.CrateInfo().GetSoftwareVersion())

    for fname in args.inputFiles:
        try:
            info = Usd.CrateInfo.Open(fname)
            if not info:
                Err('Failed to read %s' % fname)
                continue
        except Exception as e:
            Err('Failed to read %s\n %s' % (fname,e))
            continue
        PrintReport(fname, info, args.summary)
            
if __name__ == "__main__":
    # Restore signal handling defaults to allow output redirection and the like.
    import platform
    if platform.system() != 'Windows':
        import signal
        signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    main()
