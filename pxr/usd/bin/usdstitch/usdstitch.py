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
import argparse, os, sys
from pxr import UsdUtils, Sdf, Tf 

parser = argparse.ArgumentParser( \
            prog=sys.argv[0],
            description='Stitch multiple usd file(s) together '
                        'into one. Opinion strength is determined by '
                        'the order in which the file is given, with the '
                        'first file getting the highest strength. '
                        'The exception to this general behavior is in '
                        'time samples, which are merged together as a '
                        'union across the input layers. It is consistent '
                        'in that, if two time sample keys conflict, the '
                        'strong layer takes precedence.')

parser.add_argument('usdFiles', nargs='+')
parser.add_argument('-o', '--out', action='store',
                    help='specify a file to write out to')
results = parser.parse_args()
assert results.out is not None, "must specify output file"

if os.path.isfile(results.out):
    print "Warning: overwriting pre-existing file"

# fold over the files, left file takes precedence on op. strength
outLayer = Sdf.Layer.CreateNew(results.out)

# open up all of the files into memory before stitching.
# if one is missing, this will allow us to fail without doing
#
# try opening all files
openedFiles = [Sdf.Layer.FindOrOpen(fname) for fname in results.usdFiles]
# grab the index of all, if any, files which failed to open
unopened = [i for i, unopened in enumerate(openedFiles) if unopened is None]
# grab the filenames of the failed files for error messaging
erroredFiles = ' '.join([results.usdFiles[i] for i in unopened])
# if we failed to open any files, error out
assert len(unopened) == 0, 'unable to open file(s) %s' %erroredFiles
    
# the extra computation and fail more gracefully
try:
    for usdFile in openedFiles:
        UsdUtils.StitchLayers(outLayer, usdFile)
        outLayer.Save()
# if something in the authoring fails, remove the output file
except Exception as e:
    print 'Failed to complete stitching, removing output file %s' % results.out
    print e
    os.remove(results.out) 
