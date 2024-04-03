#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from __future__ import print_function
from pxr import Ar, Sdf, Usd, UsdUtils

import argparse, contextlib, sys

@contextlib.contextmanager
def stream(path, *args, **kwargs):
    if path == '-':
        yield sys.stdout
    else:
        with open(path, *args, **kwargs) as fp:
            yield fp

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('usdzFile')
    parser.add_argument('assetPath')
    parser.add_argument('-l', '--listContents', type=str, default='-', 
                        dest='outfile')
    parser.add_argument('-n', '--firstLayerName', type=str, default='', 
                        dest='rename')
    parser.add_argument('--arkit', dest='arkit', action='store_true')
    parser.add_argument('--check', dest='check', action='store_true')
    parser.add_argument('--numFailedChecks', dest='numFailedChecks', default=0,
                        type=int, action='store')
    parser.add_argument('--numErrors', dest='numErrors', default=0,
                        type=int, action='store')
    parser.add_argument('--editLayersInPlace', dest='editLayersInPlace',
                        default=False, action='store_true')
    parser.add_argument('--expectedDirtyLayers', type=str, default='', 
                        dest='expectedDirtyLayers')

    args = parser.parse_args()

    context = Ar.GetResolver().CreateDefaultContextForAsset(args.assetPath)
    with Ar.ResolverContextBinder(context):
        assetPath = Sdf.AssetPath(args.assetPath)
        layers, _, _ = UsdUtils.ComputeAllDependencies(assetPath)

        expectedDirtyLayers = []
        if args.expectedDirtyLayers:
            for expectedDirtyLayer in args.expectedDirtyLayers.split(','):
                layer = Sdf.Layer.FindOrOpen(expectedDirtyLayer.strip())
                assert layer is not None
                assert layer in layers
                expectedDirtyLayers.append(layer)

        if not args.arkit:
            assert UsdUtils.CreateNewUsdzPackage(assetPath, 
                    args.usdzFile, args.rename if args.rename else '',
                    args.editLayersInPlace)
        else:
            assert UsdUtils.CreateNewARKitUsdzPackage(
                    assetPath, args.usdzFile,
                    args.rename if args.rename else '',
                    args.editLayersInPlace)

        for layer in layers:
            layerShouldBeDirty = layer in expectedDirtyLayers
            assert layer.dirty is layerShouldBeDirty

    zipFile = Usd.ZipFile.Open(args.usdzFile)
    assert zipFile

    with stream(args.outfile, 'w') as ofp:
        for fileName in zipFile.GetFileNames():
            print(fileName, file=ofp)

    # Validate that the usdz file can be opened on a stage.
    stage = Usd.Stage.Open(args.usdzFile)
    assert stage

    if args.check:
        rootLayerPath = stage.GetRootLayer().realPath
        context = Ar.GetResolver().CreateDefaultContextForAsset(rootLayerPath)
        with Ar.ResolverContextBinder(context):
            checker = UsdUtils.ComplianceChecker(arkit=args.arkit, verbose=True)
            checker.CheckCompliance(args.usdzFile)
            failedChecks = checker.GetFailedChecks()
            errors = checker.GetErrors()
            for msg in failedChecks + errors:
                print(msg, file=sys.stderr)
            assert args.numFailedChecks == len(failedChecks)
            assert args.numErrors == len(errors)

    sys.exit(0)
