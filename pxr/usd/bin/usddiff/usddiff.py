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

# PLEASE NOTE: This utility may be open sourced, please hesitate to add new
# dependencies.

import os, sys

# generates a command list representing a call which will generate
# a temporary ascii file used during diffing. 
def _generateCatCommand(usdcatCmd, filePath, outPath):
    if os.stat(filePath).st_size > 0:
        return [usdcatCmd, filePath, '--out', outPath]
    return ['cat', filePath, '--out', outPath]

# looks up a suitable diff tool, and locates usdcat
def _findDiffTools():
    from distutils.spawn import find_executable
    usdcatCmd = find_executable("usdcat")
    if not usdcatCmd:
        sys.exit("Error: Couldn't find 'usdcat'. Expected it to be in PATH ")

    # prefer USD_DIFF, then DIFF, else 'diff' 
    diffCmd = (os.environ.get('USD_DIFF') or 
               os.environ.get('DIFF') or 
               find_executable('diff'))
    if not diffCmd:
        sys.exit("Error: Failed to find suitable diff tool. "
                 "Expected $USD_DIFF/$DIFF to be set, or 'diff' "
                 "to be installed.")

    return (usdcatCmd, diffCmd)

def main():
    import argparse
    parser = argparse.ArgumentParser(prog=os.path.basename(sys.argv[0]),
                description="Compares two usd files using a selected diff"
                            " program. This is chosen by looking at the" 
                            " $USD_DIFF environment variable. If this is unset,"
                            " it will consult the $DIFF environment variable. "
                            " Lastly, if neither of these is set, it will try" 
                            " to use the canonical unix program, diff."
                            " This will relay the exit code of the selected"
                            " diff program.")
    parser.add_argument('baseline', 
                        help='The baseline usd file to diff against.')
    parser.add_argument('comparison',
                        help='The usd file to compare against the baseline')
    results = parser.parse_args()

    import tempfile
    with tempfile.NamedTemporaryFile(suffix='.usda') as usda1, \
         tempfile.NamedTemporaryFile(suffix='.usda') as usda2:

        usdcatCmd, diffCmd = _findDiffTools()

        # create the temporary ascii usda files to diff
        from subprocess import call
        call(_generateCatCommand(usdcatCmd, results.baseline, usda1.name)) 
        call(_generateCatCommand(usdcatCmd, results.comparison, usda2.name)) 

        # use sys exit to relay result of diff command 
        sys.exit(call([diffCmd, usda1.name, usda2.name]))

if __name__ == "__main__":
    main()
