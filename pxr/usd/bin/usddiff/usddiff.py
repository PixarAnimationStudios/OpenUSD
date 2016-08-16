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
import os, sys, shutil
from subprocess import call

# generates a command list representing a call which will generate
# a temporary ascii file used during diffing. 
def _generateCatCommand(usdcatCmd, inPath, outPath, fmt=None):
    if os.stat(inPath).st_size > 0:
        command = [usdcatCmd, inPath, '--out', outPath]

        if fmt and os.path.splitext(outPath)[1] == '.usd':
            command.append('--usdFormat')
            command.append(fmt)
        
        return command

    return ['cat', inPath, '--out', outPath]

# looks up a suitable diff tool, and locates usdcat
def _findDiffTools():
    from distutils.spawn import find_executable
    usdcatCmd = find_executable("usdcat")
    if not usdcatCmd:
        sys.exit("Error: Couldn't find 'usdcat'. Expected it to be in PATH")

    # prefer USD_DIFF, then DIFF, else 'diff' 
    diffCmd = (os.environ.get('USD_DIFF') or 
               os.environ.get('DIFF') or 
               find_executable('diff'))
    if not diffCmd:
        sys.exit("Error: Failed to find suitable diff tool. "
                 "Expected $USD_DIFF/$DIFF to be set, or 'diff' "
                 "to be installed.")

    return (usdcatCmd, diffCmd)

def _getFileFormat(path):
    from pxr import Sdf

    # Note that python's os.path.splitext retains the '.' portion
    # when obtaining an extension, but Sdf's Fileformat API doesn't 
    # expect one.
    _, ext = os.path.splitext(path)
    fileFormat = Sdf.FileFormat.FindByExtension(ext[1:])

    if fileFormat and fileFormat.CanRead(path):
        return fileFormat.formatId

    return None

def _convertTo(inPath, outPath, usdcatCmd, fmt=None):
    call(_generateCatCommand(usdcatCmd, inPath, outPath, fmt)) 

def _cleanUpAndExit(tempDir, exitCodeOrErrorMsg):
    shutil.rmtree(tempDir)

    if isinstance(exitCodeOrErrorMsg, str):
        exitCodeOrErrorMsg = "Error: " + exitCodeOrErrorMsg

    sys.exit(exitCodeOrErrorMsg)

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
    parser.add_argument('-n', '--noeffect', action='store_true',
                        help='Do not edit either file.') 
    results = parser.parse_args()

    import tempfile
    tempDir = tempfile.mkdtemp()

    # Generate unique identifiers for our files in the temp dir
    # and convert them explicitly to usda
    tempBaselineFileName = ("baseline_" + 
        os.path.splitext(os.path.basename(results.baseline))[0] + '.usda')
    tempComparisonFileName = ("comparison_" +
        os.path.splitext(os.path.basename(results.comparison))[0] + '.usda')
    tempBaselinePath = os.path.join(tempDir, tempBaselineFileName)
    tempComparisonPath = os.path.join(tempDir, tempComparisonFileName)

    with open(tempBaselinePath, "w+") as tempBaseline, \
         open(tempComparisonPath, "w+") as tempComparison:  

        usdcatCmd, diffCmd = _findDiffTools()
        baselineFileType = _getFileFormat(results.baseline)
        comparisonFileType = _getFileFormat(results.comparison)

        pluginError = 'Cannot find supported file format plugin for %s'
        if baselineFileType is None:
            _cleanUpAndExit(tempDir, pluginError % results.baseline)

        if comparisonFileType is None:
            _cleanUpAndExit(tempDir, pluginError % results.comparison)

        # Dump the contents of our files into the temporaries
        _convertTo(results.baseline, tempBaseline.name, usdcatCmd)
        _convertTo(results.comparison, tempComparison.name, usdcatCmd)

        tempBaselineTimestamp = os.path.getmtime(tempBaseline.name)
        tempComparisonTimestamp = os.path.getmtime(tempComparison.name)

        diffResult = call([diffCmd, tempBaseline.name, tempComparison.name])

        tempBaselineChanged = ( 
            os.path.getmtime(tempBaseline.name) != tempBaselineTimestamp)
        tempComparisonChanged = (
            os.path.getmtime(tempComparison.name) != tempComparisonTimestamp)

        # If we intend to edit either of the files
        if not results.noeffect:
            accessError = 'Cannot write to %s, insufficient permissions' 
            if tempBaselineChanged:
                if not os.access(results.baseline, os.W_OK):
                    _cleanUpAndExit(tempDir, accessError % results.baseline)

                _convertTo(tempBaseline.name, results.baseline,
                           usdcatCmd, baselineFileType)

            if tempComparisonChanged:
                if not os.access(results.comparison, os.W_OK):
                    _cleanUpAndExit(tempDir, accessError % results.comparison)

                _convertTo(tempComparison.name, results.comparison,
                           usdcatCmd, comparisonFileType)

    _cleanUpAndExit(tempDir, diffResult)

if __name__ == "__main__":
    main()
