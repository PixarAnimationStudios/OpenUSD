#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

import argparse, sys, signal, os
from pxr import Sdf, Usd

def Err(msg):
    print >>sys.stderr, 'Error: ' + msg

def PrintReport(fname, info, summaryOnly):
    print '@%s@' % fname, 'file version', info.GetFileVersion()
    ss = info.GetSummaryStats()
    print ('  %s specs, %s paths, %s tokens, %s strings, '
           '%s fields, %s field sets' % 
           (ss.numSpecs, ss.numUniquePaths, ss.numUniqueTokens,
            ss.numUniqueStrings, ss.numUniqueFields, ss.numUniqueFieldSets))
    if summaryOnly:
        return
    print '  Structural Sections:'
    for sec in info.GetSections():
        print '    %16s %16d bytes at offset 0x%X' % (
            sec.name, sec.size, sec.start)
    print ''

def main():
    # restore nix signal handling defaults to allow output
    # redirection and the like.
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    parser = argparse.ArgumentParser(
        prog=os.path.basename(sys.argv[0]), 
        description='Write information about a usd crate (usdc) file to stdout')

    parser.add_argument('inputFiles', nargs='+')
    parser.add_argument('-s', '--summary', action='store_true', 
                        help='report only a short summary')

    args = parser.parse_args()

    print 'Usd crate software version', Usd.CrateInfo().GetSoftwareVersion()

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
    main()
