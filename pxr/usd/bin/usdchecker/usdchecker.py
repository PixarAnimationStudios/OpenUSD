#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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
import argparse
import itertools
import sys

from contextlib import contextmanager

from pxr import Ar, Sdf, Usd, UsdGeom, UsdShade, UsdUtils

class TermColors:
    WARN = '\033[93m'
    FAIL = '\033[91m'
    END = '\033[0m'

@contextmanager
def _Stream(path, *args, **kwargs):
    if path == '-':
        yield sys.stdout
    else:
        with open(path, *args, **kwargs) as fp:
            yield fp

def _Print(stream, msg):
    print >>stream, msg

def _Err(msg):
    sys.stderr.write(TermColors.FAIL + msg + TermColors.END + '\n')

def _Warn(msg):
    sys.stderr.write(TermColors.WARN + msg + TermColors.END + '\n')

def _IsPackageOrPackagedLayer(layer):
    return layer.GetFileFormat().IsPackage() or \
           Ar.IsPackageRelativePath(layer.identifier)

class ComplianceChecker:
    _allowedLayerFormatIds = ('usd', 'usda', 'usdc', 'usdz')
    _allowedImageFormats = ("jpg", "png")
    _allowedFileExtensions = _allowedLayerFormatIds + _allowedImageFormats

    _unsupportedImageFormats = ["bmp", "tga", "hdr", "exr", "tif", "zfile", 
                                "tx"]
    # XXX: Do we want to allow the core Skel and Lux types?
    _arkitAllowedPrimTypeNames = ('', 'Scope', 'Xform', 'Camera',
                                  'Shader', 'Material',
                                  'Mesh', 'Sphere', 'Cube', 'Cylinder', 'Cone',
                                  'Capsule', 'GeomSubset', 'NurbsPatch', 
                                  'Points')
    _allowedPrimTypeNames = _arkitAllowedPrimTypeNames + ('BasisCurves', 
            'NurbsCurves', 'NodeGraph')

    def Success(self):
        return len(self._failedChecks) == 0

    def GetFailedChecks(self):
        return self._failedChecks

    def _Msg(self, msg):
        if self._verbose:
            print msg
    
    def _AddFailedCheck(self, msg):
        """ Adds msg to the list of all failed checks.    
        """ 
        self._failedChecks.append(msg)

    def __init__(self, inputFile, arkit, rootPackageOnly, skipVariants, verbose):
        self._failedChecks = []
        self._doVariants = not skipVariants
        self._rootPackageOnly = rootPackageOnly
        self._arkit = arkit
        self._verbose = verbose
        self._checkedPackages = set()

        # Collect all warnings using a diagnostic delegate.
        delegate = UsdUtils.CoalescingDiagnosticDelegate()
        usdStage = Usd.Stage.Open(inputFile)
        allDiagnostics = delegate.TakeUncoalescedDiagnostics()
        for diag in allDiagnostics:
            # "_ReportErrors" is the name of the function that issues warnings about 
            # unresolved references, sublayers and other composition arcs.
            if '_ReportErrors' in diag.sourceFunction and \
                'usd/stage.cpp' in diag.sourceFileName:
                self._AddFailedCheck(diag.commentary)

        with Ar.ResolverContextBinder(usdStage.GetPathResolverContext()):
            # This recursively computes all of inputFiles's external 
            # dependencies.
            (allLayerDeps, allAssetDeps, unresolvedPaths) = \
                    UsdUtils.ComputeAllDependencies(Sdf.AssetPath(inputFile))
            self._CheckDependencies(usdStage, allLayerDeps, allAssetDeps, 
                                    unresolvedPaths)
            self._CheckStage(usdStage, allLayerDeps)

    def _CheckDependencies(self, usdStage, 
                           layerDeps, assetDeps, unresolvedPaths):
        # Process every package just once by storing them all in a set.
        packages = set()
        for layer in layerDeps:
            if _IsPackageOrPackagedLayer(layer):
                packagePath = Ar.SplitPackageRelativePathInner(
                        layer.identifier)[0]
                packages.add(packagePath)
            self._CheckLayer(layer)
        for package in packages:
            self._CheckPackage(package)

        # If the root layer is a package, validate that all the loaded layers
        # belong to the package.
        rootLayer = usdStage.GetRootLayer()
        if _IsPackageOrPackagedLayer(rootLayer):
            packagePath = usdStage.GetRootLayer().realPath
            if packagePath:
                if Ar.IsPackageRelativePath(packagePath):
                    packagePath = Ar.SplitPackageRelativePathOuter(
                            packagePath)[0]
                for layer in layerDeps:
                    # In-memoery layers that session layers won't have a real 
                    # path. We can skip them.
                    if layer.realPath:
                        if not layer.realPath.startswith(packagePath):
                            self._AddFailedCheck("Found loaded layer '%s' that "
                                "does not belong to the package '%s'." % 
                                (layer.identifier, packagePath))
                for asset in assetDeps:
                    if not asset.startswith(packagePath):
                        self._AddFailedCheck("Found asset reference '%s' that "
                            "does not belong to the package '%s'." % 
                            (asset, packagePath))

        for unresolvedPath in unresolvedPaths:
            self._AddFailedCheck("Found unresolvable external dependency '%s'."
                                 % unresolvedPath)

    def _CheckStage(self, usdStage, allLayers):
        if self._arkit:
            self._CheckARKitCompatibility(usdStage, allLayers)

        if self._rootPackageOnly:
            self._CheckRootPackage(usdStage)
            return

        upAxis = UsdGeom.GetStageUpAxis(usdStage)
        if upAxis != UsdGeom.Tokens.y:
            self._AddFailedCheck("Stage has upAxis '%s'. upAxis should be '%s'." 
                                 % (upAxis, UsdGeom.Tokens.y))

        # Author all variant switches in the session layer.
        usdStage.SetEditTarget(usdStage.GetSessionLayer())
        allPrimsIt = iter(Usd.PrimRange.Stage(usdStage, 
                                            Usd.TraverseInstanceProxies()))
        self._TraverseRange(allPrimsIt, isStageRoot=True)

    def _CheckARKitCompatibility(self, usdStage, allLayers):
        layersOnDisk = [i for i in allLayers if i.realPath]
        if len(layersOnDisk) > 1:
            self._AddFailedCheck("The stage contains %s layers. It should "
                "contain a single usdc layer to be compatible with ARKit's "
                "implementation of usdz." % len(layersOnDisk))
        
        # How do we check if the root layer in the package is a usdc?
        rootLayerRealPath = usdStage.GetRootLayer().realPath
        if rootLayerRealPath.endswith(".usdz"):
            zipFile = Usd.ZipFile.Open(rootLayerRealPath)
            if not zipFile:
                _Err("Could not open package at path '%s'." % resolvedPath)
                return
            fileNames = zipFile.GetFileNames()
            if not fileNames[0].endswith(".usdc"):
                self._AddFailedCheck("First file (%s) in usdz package '%s' does "
                    "not have the .usdc extension." % (fileNames[0], 
                    rootLayerRealPath))
        elif not rootLayerRealPath.endswith(".usdc"):
            self._AddFailedCheck("Root layer of the stage '%s' does not "
                "have the '.usdc' extension." % (rootLayerRealPath))

    def _CheckShader(self, prim):
        self._Msg("Checking shader <%s>." % prim.GetPath())
        shader = UsdShade.Shader(prim)
        if not shader:
            _Err("Prim <%s> with typename 'Shader' is not a valid shader." % 
                 prim.GetPath())
            return

        implSource = shader.GetImplementationSource()
        if implSource != UsdShade.Tokens.id:
            self._AddFailedCheck("Shader <%s> has non-id implementation "
                    "source '%s'." % (prim.GetPath(), implSource))

        shaderId = shader.GetShaderId()
        if not shaderId or not shaderId.startswith('Usd'):
            self._AddFailedCheck("Shader <%s> has unsupported info:id '%s'." % 
                    (prim.GetPath(), shaderId))

        shaderInputs = shader.GetInputs()
        for ip in shaderInputs:
            if ip.GetTypeName() == Sdf.ValueTypeNames.Asset:
                texFilePath = str(ip.Get()).strip('@')
                self._CheckTexture(texFilePath)
            elif ip.GetTypeName() == Sdf.ValueTypeNames.AssetArray:
                texPathArray = ip.Get()
                texPathArray = [str(i).strip('@') for i in texPathArray]
                for texPath in texPathArray:
                    self._CheckTexture(texFilePath)

    def _CheckPrim(self, prim):
        self._Msg("Checking prim <%s>." % prim.GetPath())

        # XXX: check all applied API scheams and make sure that only 
        # the core ones are present.

        if prim.GetTypeName() not in (
                ComplianceChecker._arkitAllowedPrimTypeNames if self._arkit
                else ComplianceChecker._allowedPrimTypeNames):
            self._AddFailedCheck("Prim <%s> has unsupported type '%s'." % 
                                (prim.GetPath(), prim.GetTypeName()))
        
        if prim.GetTypeName() == 'Shader':
            self._CheckShader(prim)
    
    def _CheckPackage(self, packagePath):
        self._Msg("Checking package <%s>." % packagePath)
        # XXX: Should we open the package on a stage to ensure that it is valid 
        # and entirely self-contained.

        pkgExt = Ar.GetResolver().GetExtension(packagePath)
        if pkgExt != "usdz":
            self._AddFailedCheck("Package at path %s has an invalid extension." 
                    % packagePath)
            return

        # Check the parent package first.
        if Ar.IsPackageRelativePath(packagePath):
            parentPackagePath = Ar.SplitPackageRelativePathInner(packagePath)[0]
            self._CheckPackage(parentPackagePath)
        
        # Avoid checking the same parent package multiple times.
        if packagePath in self._checkedPackages:
            return
        self._checkedPackages.add(packagePath)

        resolvedPath = Ar.GetResolver().Resolve(packagePath)
        if len(resolvedPath) == 0:
            _Err("Failed to resolve package path '%s'." % packagePath)
            return

        zipFile = Usd.ZipFile.Open(resolvedPath)
        if not zipFile:
            _Err("Could not open package at path '%s'." % resolvedPath)
            return

        fileNames = zipFile.GetFileNames()
        for fileName in fileNames:
            fileExt = Ar.GetResolver().GetExtension(fileName)
            if fileExt not in ComplianceChecker._allowedFileExtensions:
                self._AddFailedCheck("File '%s' in package '%s' has unknown or "
                    "unsupported extension '%s'." % (fileName, packagePath, 
                    fileExt))
            fileInfo = zipFile.GetFileInfo(fileName)
            offset = fileInfo.dataOffset 
            if offset % 64 != 0:
                self._AddFailedCheck("File '%s' in package '%s' has an invalid "
                        "offset %s." % (fileName, packagePath, offset))
            if fileInfo.compressionMethod != 0:
                self._AddFailedCheck("File '%s' in package '%s' has "
                    "compression. Compression method is '%s', actual size is "
                    "%s. Uncompressed size is %s." % (
                    fileName, packagePath, fileInfo.compressionMethod,
                    fileInfo.size, fileInfo.uncompressedSize))

    def _CheckLayer(self, layer):
        self._Msg("Checking layer <%s>." % layer.identifier)
        formatId = layer.GetFileFormat().formatId
        if not formatId in ComplianceChecker._allowedLayerFormatIds:
            self._AddFailedCheck("Layer '%s' has unsupported formatId '%s'." % 
                    (layer.identifier, formatId))

    def _CheckTexture(self, texAssetPath):
        self._Msg("Checking texture <%s>." % texAssetPath)
        texFileExt = Ar.GetResolver().GetExtension(texAssetPath)
        if texFileExt in ComplianceChecker._unsupportedImageFormats:
            self._AddFailedCheck("Found texture file '%s' with unsupported "
                    "file format." % texAssetPath)
        elif texFileExt not in ComplianceChecker._allowedImageFormats:
            self._AddFailedCheck("Found texture file '%s' with unknown file "
                    "format." % texAssetPath)

    def _TraverseRange(self, primRangeIt, isStageRoot):
        primsWithVariants = []
        success = True
        rootPrim = primRangeIt.GetCurrentPrim()
        for prim in primRangeIt:
            # Skip variant set check on the root prim if it is the stage'.
            if not self._doVariants or (not isStageRoot and prim == rootPrim):
                success = self._CheckPrim(prim) and success
                continue

            vSets = prim.GetVariantSets()
            vSetNames = vSets.GetNames()
            if len(vSetNames) == 0:
                success = self._CheckPrim(prim) and success
            else:
                primsWithVariants.append(prim)
                primRangeIt.PruneChildren()

        for prim in primsWithVariants:
            success = self._TraverseVariants(prim) and success

        return success

    def _TraverseVariants(self, prim):
        if prim.IsInstanceProxy():
            return True

        vSets = prim.GetVariantSets()
        vSetNames = vSets.GetNames()
        allVariantNames = []
        for vSetName in vSetNames:
            vSet = vSets.GetVariantSet(vSetName)
            vNames = vSet.GetVariantNames()
            allVariantNames.append(vNames)
        allVariations = itertools.product(*allVariantNames)

        for variation in allVariations:
            self._Msg("Testing variation %s of prim <%s>" % 
                      (variation, prim.GetPath()))
            for (idx, sel) in enumerate(variation):
                vSets.SetSelection(vSetNames[idx], sel)
            primRangeIt = iter(Usd.PrimRange(prim, 
                    Usd.TraverseInstanceProxies()))
            self._TraverseRange(primRangeIt, isStageRoot=False)

    def _CheckRootPackage(self, usdStage):
        rootLayer = usdStage.GetRootLayer()
        if rootLayer.GetFileFormat().IsPackage():
            packagePath = Ar.SplitPackageRelativePathInner(
                    rootLayer.identifier)[0]
            self._CheckPackage(packagePath)
        else:
            _Err("Root layer of the USD stage (%s) doesn't belong to a "
                "package!" % Usd.Describe(usdStage))


def main():
    parser = argparse.ArgumentParser(description='Utility for checking the '
        'compliance of a given USD stage or a USDZ package.')

    parser.add_argument('inputFile', type=str, 
                        help='Name of the input file to inspect.')
    parser.add_argument('-s', '--skipVariants', dest='skipVariants',
                        action='store_true', help='If specified, prims in all '
                        'possible combinations of variant selections are '
                        'validated.')
    parser.add_argument('-p', '--rootPackageOnly', dest='rootPackageOnly', 
                        action="store_true", help='Check only the specified'
                        'package. Nested packages, dependencies and their '
                        'contents are not validated.')
    parser.add_argument('-o', '--out', dest='outFile', type=str, nargs='?',
                        default='-', help='The file to which all the failed '
                        'checks are output. If unspecified, the failed checks '
                        'are output to stdout.')
    parser.add_argument('--arkit', dest='arkit', action='store_true', 
                        help='Check if the given USD stage is compatible with '
                        'ARKit\'s initial implementation of usdz. These assets '
                        'operate under greater constraints that usdz files for '
                        'more general in-house uses, and this option attempts '
                        'to ensure that these constraints are met.')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true',
                        help='Enable verbose mode.')

    args = parser.parse_args()
    inputFile = args.inputFile
    outFile = args.outFile

    if not Usd.Stage.IsSupportedFile(inputFile):
        _Err("Cannot open file '%s' on a USD stage." % args.inputFile)
        return 1
    
    checker = ComplianceChecker(inputFile, args.arkit, args.rootPackageOnly, 
                                args.skipVariants, args.verbose)

    failedChecks = checker.GetFailedChecks()
    if len(failedChecks) > 0:
        with _Stream(outFile, 'w') as ofp:
            for failedCheck in failedChecks:
                if outFile == '-':
                    failedCheck = TermColors.FAIL + failedCheck + TermColors.END
                _Print(ofp, failedCheck)

    success = checker.Success()
    print("Success!" if success else "Failed!")

    return 0 if success else 1

if __name__ == '__main__':
    sys.exit(main())
