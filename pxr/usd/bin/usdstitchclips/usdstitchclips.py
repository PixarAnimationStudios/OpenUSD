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

parser = argparse.ArgumentParser(
            prog=os.path.basename(sys.argv[0]),
            description='Stitch multiple usd file(s) together '
                        'into one using value clips. '
                        'An example call is: '
                        'usdstitchclips --out result.usd --clipPath '
                        '/World/fx/Particles_Splash clip1.usd clip2.usd '
                        '\n\n'
                        'This will produce two files, a result.topology.usd '
                        'and a result.usd.')

parser.add_argument('usdFiles', nargs='+')
parser.add_argument('-o', '--out', action='store',
                    help='specify a file to write out to')
parser.add_argument('-c', '--clipPath', action='store',
                    help='''specify a prim path to stitch clip data at. ''')
parser.add_argument('-s', '--startTimeCode', action='store',
                    help='specify a start time')
parser.add_argument('-r', '--stride', action='store',
                    help='specify a stride for template metadata')
parser.add_argument('-e', '--endTimeCode', action='store',
                    help='specify an end time')
parser.add_argument('-t', '--templateMetadata', action='store_true',
                    help='author template clip metadata in the root layer.')
parser.add_argument('-p', '--templatePath', action='store',
                    help='specify a template asset path to author') 
parser.add_argument('--clipSet', action='store',
                    help='specify a clipSet to author clip metadata under.')
parser.add_argument('--activeOffset', action='store', required=False,
                    help='specify an active offset')
# useful for debugging with diffs
parser.add_argument('-n', '--noComment', action='store_true',
                    help='''do not write a comment specifying how the
                         usd file was generated''')
results = parser.parse_args()

# verify the integrity of the inputs
assert results.out is not None, "must specify output file(--out)"
assert results.clipPath is not None, "must specify a clip path(--clipPath)"
assert results.usdFiles is not None, "must specify clip files"

if os.path.isfile(results.out):
    print "Warning: merging with current result layer"

outLayerGenerated = False
topologyLayerGenerated = False
topologyLayerName = ""

try:
    outLayer = Sdf.Layer.FindOrOpen(results.out)
    if not outLayer:
        outLayerGenerated = True
        outLayer = Sdf.Layer.CreateNew(results.out)

    topologyLayerName = UsdUtils.GenerateClipTopologyName(results.out)
    topologyLayer = Sdf.Layer.FindOrOpen(topologyLayerName)
    if not topologyLayer:
        topologyLayerGenerated = True
        topologyLayer = Sdf.Layer.CreateNew(topologyLayerName)

    if results.startTimeCode:
        results.startTimeCode = float(results.startTimeCode)

    if results.endTimeCode:
        results.endTimeCode = float(results.endTimeCode)

    if results.stride:
        results.stride = float(results.stride)

    if results.activeOffset:
        results.activeOffset = float(results.activeOffset)

    if results.templateMetadata:
        def _checkMissingTemplateArg(argName, argValue):
            if not argValue:
                raise Tf.ErrorException('Error: %s must be specified '
                                        'when --templateMetadata is' % argName)

        _checkMissingTemplateArg('templatePath', results.templatePath)
        _checkMissingTemplateArg('endTimeCode', results.endTimeCode)
        _checkMissingTemplateArg('startTimeCode', results.startTimeCode)
        _checkMissingTemplateArg('stride', results.stride)

        UsdUtils.StitchClipsTopology(topologyLayer, results.usdFiles)
        UsdUtils.StitchClipsTemplate(outLayer, 
                                     topologyLayer,
                                     results.clipPath,
                                     results.templatePath,
                                     results.startTimeCode,
                                     results.endTimeCode,
                                     results.stride,
                                     results.activeOffset,
                                     results.clipSet)
    else:
        if results.templatePath:
            raise Tf.ErrorException('Error: templatePath cannot be specified '
                                    'without --templateMetadata')
        if results.activeOffset:
            raise Tf.ErrorException('Error: activeOffset cannot be specified '
                                    'without --templateMetadata')
        if results.stride:
            raise Tf.ErrorException('Error: stride cannot be specified '
                                    'without --templateMetadata')

        UsdUtils.StitchClips(outLayer, results.usdFiles, results.clipPath, 
                             results.startTimeCode, results.endTimeCode,
                             results.clipSet)


    if not results.noComment:
        outLayer.comment = 'Generated with ' + ' '.join(sys.argv)
        outLayer.Save()

except Tf.ErrorException as e:
    # if something in the authoring fails, remove the output file 
    if outLayerGenerated and os.path.isfile(results.out):
        os.remove(results.out)
    if topologyLayerGenerated and os.path.isfile(topologyLayerName):
        os.remove(topologyLayerName)
    sys.exit(e)
