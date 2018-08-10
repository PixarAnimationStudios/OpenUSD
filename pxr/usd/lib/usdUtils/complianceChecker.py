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


# A utility class for checking compliance of a given USD asset or a USDZ 
# package.
# Because usdz files are zip files, someone could use generic zip tools to 
# create an archive and just change the extension, producing a .usdz file that 
# does not honor the additional constraints that usdz files require.  Even if 
# someone does use our official archive creation tools, though, we intentionally 
# allow creation of usdz files that can be very "permissive" in their contents, 
# for internal studio uses, where portability outside the studio is not a 
# concern.  For content meant to be delivered over the web (eg. ARKit assets), 
# however, we must be much more restrictive.
# 
# This class provides two levels of compliance checking: 
# * "simple or structural" validation that primarily verifies the files within 
# the package are:
#       * laid out properly
#       * aren't compressed or encrypted and 
#       * only contains usd and texture file formats that can be consumed 
#       directly from within the package (e.g. no abc files allowed)
#       * contain no unresolvable paths or paths that resolve to assets outside 
#       the root package.
#       
# * "ARKit" compatibility level, which brings in many more restrictions:
#       * No file formats other than the core-supported ones.
#       * No image file formats other than jpg and png.
#       * no use of custom schemas not provided by core USD and no 
#       PointInstancers.
#       * The stage must be Y-up.
#       * no shader nodes with non-id implementatinSource and no shader nodes 
#       other than the Usd* nodes used for preview shading.
#       * 
#
class ComplianceChecker(object):
    # Only core USD file formats are allowed.
    _allowedLayerFormatIds = ('usd', 'usda', 'usdc', 'usdz')

    # Allow just png and jpg for now.
    _allowedImageFormats = ("jpg", "png")

    # Only layer and image files are allowed.
    _allowedFileExtensions = _allowedLayerFormatIds + _allowedImageFormats

    # Include a list of "unsupported" image formats to provide better error
    # messages whwn we find one of these.
    _unsupportedImageFormats = ["bmp", "tga", "hdr", "exr", "tif", "zfile", 
                                "tx"]

    # All core prim types other than UsdGeomPointInstancers and the type in 
    # UsdLux are allowed.
    _allowedPrimTypeNames = ('', 'Scope', 'Xform', 'Camera',
                             'Shader', 'Material',
                             'Mesh', 'Sphere', 'Cube', 'Cylinder', 'Cone',
                             'Capsule', 'GeomSubset', 'NurbsPatch', 
                             'Points', 'SkelRoot', 'Skeleton', 
                             'SkelAnimation', 'BlendShape')

    @staticmethod
    def GetRules(arkit=False):
        baseRules = [
            # 1
            "Files within a usdz package must be laid out properly, i.e. they"
            "should be aligned to 64 byptes.",

            # 2
            "Files withing a usdz package should not be compressed or "
            "encrypted.",

            # 3
            "Texture files should be .jpg or .png.",
        ]

        arkitRules = [
            # 4
            "The composed USD stage should not contain any unresolvable asset "
            "dependencies (in every possible variation of the asset), when "
            "using the default asset resolver. If the root layer is a package, "
            "then the composed stage should not contain references to files "
            "outside the package. In other words, the package should be "
            "entirely self-contained.",

            # 5
            "All included layers that participate in composition should have "
            "one of the core supported file formats.",
            
            # 6
            "UsdGeomPointInstancers and custom schemas not provided by core "
            "USD are not allowed.",

            # 7
            "The stage must be Y-up.",
            
            # 8
            "Shader nodes must have \"id\" as their implementationSource with "
            "id values that begin with \"Usd*\".",

            # 9
            "The root layer of the package must be a usdc file and must not "
            "include any external dependencies that are USD layers."
        ]

        allRules = baseRules
        if arkit:
            allRules += arkitRules

        return allRules

    @staticmethod
    def DumpRules(arkit=False):
        print "Checking rules: "
        for ruleNum, rule in enumerate(ComplianceChecker.GetRules(arkit=arkit)):
            print "[%s] %s" % (ruleNum + 1, rule)
        print "-----------------------------------------"

    def GetFailedChecks(self):
        return self._failedChecks

    def GetErrors(self):
        return self._errors

    def _Msg(self, msg):
        if self._verbose:
            print msg
    
    def _AddError(self, errMsg):
        self._errors.append(errMsg)

    def _AddFailedCheck(self, msg, ruleNum):
        # XXX: It would be nice to have separate classes for validating 
        # each rule in the future and not have to associate the failed check 
        # with a rule number like this.
        self._failedChecks.append(msg + " (violates rule(s) %s)" % ruleNum)

        if isinstance(ruleNum, list):
            for num in ruleNum:
                self._violatedRules.add(num)
        else:
            self._violatedRules.add(ruleNum)

    def __init__(self, inputFile, 
                 arkit=False, skipARKitRootLayerCheck=False,
                 rootPackageOnly=False, 
                 skipVariants=False, verbose=False):
        self._arkit = arkit
        self._skipARKitRootLayerCheck = skipARKitRootLayerCheck

        self._rootPackageOnly = rootPackageOnly
        self._doVariants = not skipVariants
        self._verbose = verbose

        self._failedChecks = []
        self._errors = []
        self._violatedRules = set()
        self._checkedPackages = set()
        
        from pxr import Ar, Sdf, Usd, UsdUtils
        if not Usd.Stage.IsSupportedFile(inputFile):
            _AddError("Cannot open file '%s' on a USD stage." % args.inputFile)
            return

        # Collect all warnings using a diagnostic delegate.
        delegate = UsdUtils.CoalescingDiagnosticDelegate()
        usdStage = Usd.Stage.Open(inputFile)
        allDiagnostics = delegate.TakeUncoalescedDiagnostics()
        if self._arkit:
            for diag in allDiagnostics:
                # "_ReportErrors" is the name of the function that issues 
                # warnings about unresolved references, sublayers and other 
                # composition arcs.
                if '_ReportErrors' in diag.sourceFunction and \
                    'usd/stage.cpp' in diag.sourceFileName:
                    self._AddFailedCheck(diag.commentary, ruleNum=4)

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
        from pxr import Ar
        def _IsPackageOrPackagedLayer(layer):
            return layer.GetFileFormat().IsPackage() or \
                    Ar.IsPackageRelativePath(layer.identifier)

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
        if self._arkit and _IsPackageOrPackagedLayer(rootLayer):
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
                                (layer.identifier, packagePath), ruleNum=4)
                for asset in assetDeps:
                    if not asset.startswith(packagePath):
                        self._AddFailedCheck("Found asset reference '%s' that "
                            "does not belong to the package '%s'." % 
                            (asset, packagePath), ruleNum=4)

        for unresolvedPath in unresolvedPaths:
            self._AddFailedCheck("Found unresolvable external dependency '%s'."
                                 % unresolvedPath, ruleNum=4)

    def _CheckStage(self, usdStage, allLayers):
        if self._arkit:
            from pxr import UsdGeom
            if not self._skipARKitRootLayerCheck:
                self._CheckARKitCompatibility(usdStage, allLayers)
            upAxis = UsdGeom.GetStageUpAxis(usdStage)
            if upAxis != UsdGeom.Tokens.y:
                self._AddFailedCheck("Stage has upAxis '%s'. upAxis should be '%s'." 
                                    % (upAxis, UsdGeom.Tokens.y), ruleNum=7)

        if self._rootPackageOnly:
            self._CheckRootPackage(usdStage)
            return

        from pxr import Usd
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
                "implementation of usdz." % len(layersOnDisk), ruleNum=9)
        
        # How do we check if the root layer in the package is a usdc?
        rootLayerRealPath = usdStage.GetRootLayer().realPath
        if rootLayerRealPath.endswith(".usdz"):
            from pxr import Usd
            zipFile = Usd.ZipFile.Open(rootLayerRealPath)
            if not zipFile:
                self._AddError("Could not open package at path '%s'." % 
                        resolvedPath)
                return
            fileNames = zipFile.GetFileNames()
            if not fileNames[0].endswith(".usdc"):
                self._AddFailedCheck("First file (%s) in usdz package '%s' "
                    "does not have the .usdc extension." % (fileNames[0], 
                    rootLayerRealPath), ruleNum=9)
        elif not rootLayerRealPath.endswith(".usdc"):
            self._AddFailedCheck("Root layer of the stage '%s' does not "
                "have the '.usdc' extension." % (rootLayerRealPath),
                ruleNum=9)

    def _CheckShader(self, prim):
        from pxr import Sdf, UsdShade
        self._Msg("Checking shader <%s>." % prim.GetPath())
        shader = UsdShade.Shader(prim)
        if not shader:
            self._AddError("Prim <%s> with typename 'Shader' is not a valid "
                           "shader." % prim.GetPath(), ruleNum=8)
            return

        implSource = shader.GetImplementationSource()
        if implSource != UsdShade.Tokens.id:
            self._AddFailedCheck("Shader <%s> has non-id implementation "
                    "source '%s'." % (prim.GetPath(), implSource), ruleNum=8)

        shaderId = shader.GetShaderId()
        if not shaderId or not shaderId.startswith('Usd'):
            self._AddFailedCheck("Shader <%s> has unsupported info:id '%s'." % 
                    (prim.GetPath(), shaderId), ruleNum=8)

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

        # XXX: Should we check all applied API scheams and make sure that only 
        # the core ones are present.

        if self._arkit and prim.GetTypeName() not in \
                ComplianceChecker._allowedPrimTypeNames:
            self._AddFailedCheck("Prim <%s> has unsupported type '%s'." % 
                                 (prim.GetPath(), prim.GetTypeName()), 
                                 ruleNum=6)
        
        if self._arkit and prim.GetTypeName() == 'Shader':
            self._CheckShader(prim)
    
    def _CheckPackage(self, packagePath):
        self._Msg("Checking package <%s>." % packagePath)
        # XXX: Should we open the package on a stage to ensure that it is valid 
        # and entirely self-contained.

        from pxr import Ar, Usd
        pkgExt = Ar.GetResolver().GetExtension(packagePath)
        if self._arkit and pkgExt != "usdz":
            self._AddFailedCheck("Package at path %s has an invalid extension." 
                    % packagePath, ruleNum=5)
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
            self._AddError("Failed to resolve package path '%s'." % packagePath)
            return

        zipFile = Usd.ZipFile.Open(resolvedPath)
        if not zipFile:
            self._AddError("Could not open package at path '%s'." % resolvedPath)
            return

        fileNames = zipFile.GetFileNames()
        for fileName in fileNames:
            fileExt = Ar.GetResolver().GetExtension(fileName)
            if fileExt not in ComplianceChecker._allowedFileExtensions:
                self._AddFailedCheck("File '%s' in package '%s' has unknown or "
                    "unsupported extension '%s'." % (fileName, packagePath, 
                    fileExt), ruleNum=[4,5])
            fileInfo = zipFile.GetFileInfo(fileName)
            offset = fileInfo.dataOffset 
            if offset % 64 != 0:
                self._AddFailedCheck("File '%s' in package '%s' has an invalid "
                        "offset %s." % (fileName, packagePath, offset),
                        ruleNum=1)
            if fileInfo.compressionMethod != 0:
                self._AddFailedCheck("File '%s' in package '%s' has "
                    "compression. Compression method is '%s', actual size is "
                    "%s. Uncompressed size is %s." % (
                    fileName, packagePath, fileInfo.compressionMethod,
                    fileInfo.size, fileInfo.uncompressedSize),
                    ruleNum=2)

    def _CheckLayer(self, layer):
        self._Msg("Checking layer <%s>." % layer.identifier)
        formatId = layer.GetFileFormat().formatId
        if self._arkit and \
                not formatId in ComplianceChecker._allowedLayerFormatIds:
            self._AddFailedCheck("Layer '%s' has unsupported formatId '%s'." % 
                    (layer.identifier, formatId), ruleNum=5)

    def _CheckTexture(self, texAssetPath):
        from pxr import Ar
        self._Msg("Checking texture <%s>." % texAssetPath)
        texFileExt = Ar.GetResolver().GetExtension(texAssetPath)
        if texFileExt in ComplianceChecker._unsupportedImageFormats:
            self._AddFailedCheck("Found texture file '%s' with unsupported "
                    "file format." % texAssetPath, ruleNum=3)
        elif texFileExt not in ComplianceChecker._allowedImageFormats:
            self._AddFailedCheck("Found texture file '%s' with unknown file "
                    "format." % texAssetPath, ruleNum=3)

    def _TraverseRange(self, primRangeIt, isStageRoot):
        primsWithVariants = []
        rootPrim = primRangeIt.GetCurrentPrim()
        for prim in primRangeIt:
            # Skip variant set check on the root prim if it is the stage'.
            if not self._doVariants or (not isStageRoot and prim == rootPrim):
                self._CheckPrim(prim)
                continue

            vSets = prim.GetVariantSets()
            vSetNames = vSets.GetNames()
            if len(vSetNames) == 0:
                self._CheckPrim(prim)
            else:
                primsWithVariants.append(prim)
                primRangeIt.PruneChildren()

        for prim in primsWithVariants:
            self._TraverseVariants(prim)

    def _TraverseVariants(self, prim):
        from pxr import Usd
        if prim.IsInstanceProxy():
            return True

        vSets = prim.GetVariantSets()
        vSetNames = vSets.GetNames()
        allVariantNames = []
        for vSetName in vSetNames:
            vSet = vSets.GetVariantSet(vSetName)
            vNames = vSet.GetVariantNames()
            allVariantNames.append(vNames)

        import itertools
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
        from pxr import Ar
        rootLayer = usdStage.GetRootLayer()
        if rootLayer.GetFileFormat().IsPackage():
            packagePath = Ar.SplitPackageRelativePathInner(
                    rootLayer.identifier)[0]
            self._CheckPackage(packagePath)
        else:
            self._AddError("Root layer of the USD stage (%s) doesn't belong to "
                "a package!" % Usd.Describe(usdStage))
