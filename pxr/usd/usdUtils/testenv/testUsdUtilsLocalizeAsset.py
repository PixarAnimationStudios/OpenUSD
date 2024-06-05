#!/pxrpythonsubst
#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Ar, Sdf, Usd, UsdUtils

import argparse, os, shutil, sys

def test():
    parser = argparse.ArgumentParser()
    parser.add_argument('assetPath')
    parser.add_argument('localizationDir')
    parser.add_argument('-l', '--listContents', type=str, default='', 
                        dest='outfile')
    parser.add_argument('--check', dest='check', action='store_true')
    parser.add_argument('--editLayersInPlace', dest='editLayersInPlace',
                        default=False, action='store_true')
    parser.add_argument('--expectedDirtyLayers', type=str, default='', 
                        dest='expectedDirtyLayers')

    args = parser.parse_args()

    if os.path.exists(args.localizationDir):
        print("Removing existing localization directory: {}".format( \
            args.localizationDir))
        shutil.rmtree(args.localizationDir)

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


        assert UsdUtils.LocalizeAsset(assetPath, args.localizationDir, 
                                      args.editLayersInPlace)

        for layer in layers:
            layerShouldBeDirty = layer in expectedDirtyLayers
            assert layer.dirty is layerShouldBeDirty

    
    localizedAssetRoot = os.path.join(args.localizationDir, 
                                      os.path.basename(args.assetPath))

    if args.check:
        CheckLocalizedAsset(localizedAssetRoot)
    
    if args.outfile:
        WriteFileList(args.localizationDir, args.outfile)

    sys.exit(0)

# Validates that the localized asset can be opened on a stage
def CheckLocalizedAsset(localizedAssetRoot):
    stage = Usd.Stage.Open(localizedAssetRoot)
    assert stage

    rootLayerPath = stage.GetRootLayer().realPath
    context = Ar.GetResolver().CreateDefaultContextForAsset(rootLayerPath)
    with Ar.ResolverContextBinder(context):
        checker = UsdUtils.ComplianceChecker(verbose=True)
        checker.CheckCompliance(localizedAssetRoot)
        failedChecks = checker.GetFailedChecks()
        errors = checker.GetErrors()
        for msg in failedChecks + errors:
            print(msg, file=sys.stderr)
        assert len(failedChecks) == 0
        assert len(errors) == 0

def WriteFileList(localizedAssetDir, listPath):
    rootFolderPathStr = localizedAssetDir + os.sep
    contents = []
    for path, directories, files in os.walk(localizedAssetDir):
        for file in files:
            localizedPath = os.path.join(path,file)
            localizedPath = localizedPath.replace(rootFolderPathStr, '')
            localizedPath = localizedPath.replace('\\', '/')
            contents.append(localizedPath + '\n')
    
    contents.sort()

    with open(listPath, 'w') as contentsFile:
        contentsFile.writelines(contents)

if __name__ == '__main__':
    test()