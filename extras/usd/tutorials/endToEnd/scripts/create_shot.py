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

"""
Creates a shot with separate layers for departments.

"""

import os
def main():
    import optparse

    descr = __doc__.strip()
    usage = 'usage: %prog [options] shot'
    parser = optparse.OptionParser(description=descr, usage=usage)
    parser.add_option('-b', '--baseLayer', 
            help='this will be added verbatim in the subLayerPaths.')
    parser.add_option('-o', '--outputDir',
            help='directory to create shot.  if none specified, will use shotName.')

    parser.add_option('-f', '--force', default=False, action='store_true',
            help='if False, this will error if [outputDir] exists')
    options, args = parser.parse_args()

    if len(args) != 1:
        parser.error("No shot specified")

    shot = args[0]
    force = options.force

    outputDir = options.outputDir
    if not outputDir:
        outputDir = shot

    if os.path.exists(outputDir):
        if not force:
            parser.error('outputDir "%s" exists.  Use -f to override' % outputDir)
    else:
        os.makedirs(outputDir)

    _CreateShot(shot, outputDir, options.baseLayer)

def _CreateShot(shotName, shotDir, baseLayer):
    shotFilePath = os.path.join(shotDir, '%s.usd' % shotName)

    from pxr import Usd, UsdGeom
    shotStage = Usd.Stage.CreateNew(shotFilePath)
    print "Creating shot at %s" % shotFilePath

    _CreateAndAddSubLayers(shotStage, shotName, shotDir, [
        './%s_sim.usd' % shotName,
        './%s_anim.usd' % shotName,
        './%s_layout.usd' % shotName,
        './%s_sets.usd' % shotName,
        ])

    if baseLayer:
        shotStage.GetRootLayer().subLayerPaths.append(baseLayer)

    # Lets viewing applications know how to orient a free camera properly
    UsdGeom.SetStageUpAxis(shotStage, UsdGeom.Tokens.y)
    shotStage.GetRootLayer().Save()

def _CreateAndAddSubLayers(stage, shotName, shotDir, subLayers):
    # We use the Sdf API here to quickly create layers.  Also, we're using it
    # as a way to author the subLayerPaths as there is no way to do that
    # directly in the Usd API.

    from pxr import Sdf
    rootLayer = stage.GetRootLayer()
    for subLayerPath in subLayers:
        Sdf.Layer.CreateNew(os.path.join(shotDir, subLayerPath))
        rootLayer.subLayerPaths.append(subLayerPath)

    # If you want to print things out, you can do:
    #print rootLayer.ExportToString()

if __name__ == '__main__':
    main()
