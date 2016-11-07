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
import os, sys 
from tempfile import NamedTemporaryFile
from subprocess import call

# generates a command list representing a call which will generate
# a temporary ascii file used during diffing. 
def _generateCatCommand(usdcatCmd, inPath, outPath, flatten=None, fmt=None):
    if os.stat(inPath).st_size > 0:
        command = [usdcatCmd, inPath, '--out', outPath]
        if flatten:
            command.append('--flatten')

        if fmt and os.path.splitext(outPath)[1] == '.usd':
            command.append('--usdFormat')
            command.append(fmt)
        
        return command

    return ['cat', inPath]

# looks up a suitable diff tool, and locates usdcat
def _findDiffTools():
    from distutils.spawn import find_executable
    usdcatCmd = find_executable("usdcat")
    if not usdcatCmd:
        sys.exit("Error: Could not find 'usdcat'. Expected it to be in PATH")

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
    # expect one. We also make sure to prune out any version specifiers.
    _, ext = os.path.splitext(path)
    if len(ext) <= 1:
        return Sdf.FileFormat.FindByExtension('usd')

    prunedExtension = ext[1:]
    versionSpecifierPos = prunedExtension.rfind('#')
    if versionSpecifierPos != -1:
        prunedExtension = prunedExtension[:versionSpecifierPos]
     
    fileFormat = Sdf.FileFormat.FindByExtension(prunedExtension)
    if fileFormat and fileFormat.CanRead(path):
        return fileFormat.formatId

    return None

def _convertTo(inPath, outPath, usdcatCmd, flatten=None, fmt=None):
    command = _generateCatCommand(usdcatCmd, inPath, outPath, flatten, fmt)

    # We are redirecting to stdout for the case where a user
    # provides an empty file and we have to use regular 'cat'
    # which doesn't offer the redirection usdcat does through --out
    with open(outPath) as outFile:
        call(command, stdout=outFile) 

def _tryEdit(fileName, tempFileName, usdcatCmd, fileType, composed):
    if composed:
        sys.exit('Error: Cannot write out flattened result.')

    if not os.access(fileName, os.W_OK):
        sys.exit('Error: Cannot write to %s, insufficient permissions' % fileName)
    
    _convertTo(tempFileName, fileName, usdcatCmd, flatten=None, fmt=fileType)

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
    parser.add_argument('-c', '--compose', action='store_true',
                        help='Fully compose both layers as Usd Stages.')
    results = parser.parse_args()

    # Generate recognizable suffixes for our files in the temp dir
    # location of the form /temp/string__originalFileName.usda 
    # where originalFileName is the basename(no extension) of the original file.
    # This allows users to tell which file is which when diffing.
    tempBaselineFileName = ("__" + 
        os.path.splitext(os.path.basename(results.baseline))[0] + '.usda') 
    tempComparisonFileName = ("__" +     
        os.path.splitext(os.path.basename(results.comparison))[0] + '.usda')

    with NamedTemporaryFile(suffix=tempBaselineFileName) as tempBaseline, \
         NamedTemporaryFile(suffix=tempComparisonFileName) as tempComparison:

        usdcatCmd, diffCmd = _findDiffTools()
        baselineFileType = _getFileFormat(results.baseline)
        comparisonFileType = _getFileFormat(results.comparison)

        pluginError = 'Error: Cannot find supported file format plugin for %s'
        if baselineFileType is None:
            sys.exit(pluginError % results.baseline)

        if comparisonFileType is None:
            sys.exit(pluginError % results.comparison)

        # Dump the contents of our files into the temporaries
        _convertTo(results.baseline, tempBaseline.name, usdcatCmd, 
                   flatten=results.compose, fmt=None)
        _convertTo(results.comparison, tempComparison.name, usdcatCmd,
                   flatten=results.compose, fmt=None)

        tempBaselineTimestamp = os.path.getmtime(tempBaseline.name)
        tempComparisonTimestamp = os.path.getmtime(tempComparison.name)

        diffResult = call([diffCmd, tempBaseline.name, tempComparison.name])

        tempBaselineChanged = ( 
            os.path.getmtime(tempBaseline.name) != tempBaselineTimestamp)
        tempComparisonChanged = (
            os.path.getmtime(tempComparison.name) != tempComparisonTimestamp)

        # If we intend to edit either of the files
        if not results.noeffect:

            if tempBaselineChanged:
                _tryEdit(results.baseline, tempBaseline.name, 
                         usdcatCmd, baselineFileType, results.compose)

            if tempComparisonChanged:
                _tryEdit(results.comparison, tempComparison.name,
                         usdcatCmd, comparisonFileType, results.compose)

        sys.exit(diffResult)

if __name__ == "__main__":
    main()
