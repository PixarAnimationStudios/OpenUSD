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

from pxr import Ar

def _IsPackageOrPackagedLayer(layer):
    return layer.GetFileFormat().IsPackage() or \
           Ar.IsPackageRelativePath(layer.identifier)

class BaseRuleChecker(object):
    """This is Base class for all the rule-checkers."""
    def __init__(self, verbose):
        self._verbose = verbose
        self._failedChecks = []
        self._errors = []

    def _AddFailedCheck(self, msg):
        self._failedChecks.append(msg)

    def _AddError(self, msg):
        self._errors.append(msg)

    def _Msg(self, msg):
        if self._verbose:
            print msg

    def GetFailedChecks(self):
        return self._failedChecks

    def GetErrors(self):
        return self._errors

    # -------------------------------------------------------------------------
    # Virtual methods that any derived rule-checker may want to override. 
    # Default implementations do nothing.
    # 
    # A rule-checker may choose to override one or more of the virtual methods. 
    # The callbacks are invoked in the order they are defined here (i.e. 
    # CheckStage is invoked first, followed by CheckDiagnostics, followed by 
    # CheckUnresolvedPaths and so on until CheckPrim). Some of the callbacks may
    # be invoked multiple times per-rule with different parameters, for example,
    # CheckLayer, CheckPrim and CheckZipFile.

    def CheckStage(self, usdStage):
        """ Check the given usdStage. """
        pass

    def CheckDiagnostics(self, diagnostics):
        """ Check the diagnostic messages that were generated when opening the 
            USD stage. The diagnostic messages are collected using a 
            UsdUtilsCoalescingDiagnosticDelegate.
        """ 
        pass

    def CheckUnresolvedPaths(self, unresolvedPaths):
        """ Check or process any unresolved asset paths that were found when 
            analysing the dependencies.
        """
        pass

    def CheckDependencies(self, usdStage, layerDeps, assetDeps):
        """ Check usdStage's layer and asset dependencies that were gathered 
            using UsdUtils.ComputeAllDependencies().
        """
        pass

    def CheckLayer(self, layer):
        """ Check the given SdfLayer. """
        pass

    def CheckZipFile(self, zipFile, packagePath):
        """ Check the zipFile object created by opening the package at path 
            packagePath.
        """
        pass

    def CheckPrim(self, prim):
        """ Check the given prim, which may only exist is a specific combination
            of variant selections on the UsdStage.
        """
        pass

    # -------------------------------------------------------------------------

class ByteAlignmentChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "Files within a usdz package must be laid out properly, "\
                "i.e. they should be aligned to 64 bytes."

    def __init__(self, verbose):
        super(ByteAlignmentChecker, self).__init__(verbose)

    def CheckZipFile(self, zipFile, packagePath):
        fileNames = zipFile.GetFileNames()
        for fileName in fileNames:
            fileExt = Ar.GetResolver().GetExtension(fileName)
            fileInfo = zipFile.GetFileInfo(fileName)
            offset = fileInfo.dataOffset 
            if offset % 64 != 0:
                self._AddFailedCheck("File '%s' in package '%s' has an "
                        "invalid offset %s." % 
                        (fileName, packagePath, offset))

class CompressionChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "Files withing a usdz package should not be compressed or "\
               "encrypted."

    def __init__(self, verbose):
        super(CompressionChecker, self).__init__(verbose)

    def CheckZipFile(self, zipFile, packagePath):
        fileNames = zipFile.GetFileNames()
        for fileName in fileNames:
            fileExt = Ar.GetResolver().GetExtension(fileName)
            fileInfo = zipFile.GetFileInfo(fileName)
            if fileInfo.compressionMethod != 0:
                self._AddFailedCheck("File '%s' in package '%s' has "
                    "compression. Compression method is '%s', actual size "
                    "is %s. Uncompressed size is %s." % (
                    fileName, packagePath, fileInfo.compressionMethod,
                    fileInfo.size, fileInfo.uncompressedSize))

class MissingReferenceChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "The composed USD stage should not contain any unresolvable"\
            " asset dependencies (in every possible variation of the "\
            "asset), when using the default asset resolver. "

    def __init__(self, verbose):
        super(MissingReferenceChecker, self).__init__(verbose)

    def CheckDiagnostics(self, diagnostics):
        for diag in diagnostics:
            # "_ReportErrors" is the name of the function that issues 
            # warnings about unresolved references, sublayers and other 
            # composition arcs.
            if '_ReportErrors' in diag.sourceFunction and \
                'usd/stage.cpp' in diag.sourceFileName:
                self._AddFailedCheck(diag.commentary)

    def CheckUnresolvedPaths(self, unresolvedPaths):
        for unresolvedPath in unresolvedPaths:
            self._AddFailedCheck("Found unresolvable external dependency "
                    "'%s'." % unresolvedPath)

class TextureChecker(BaseRuleChecker):
    # Allow just png and jpg for now.
    _allowedImageFormats = ("jpg", "png")

    # Include a list of "unsupported" image formats to provide better error
    # messages whwn we find one of these.
    _unsupportedImageFormats = ["bmp", "tga", "hdr", "exr", "tif", "zfile", 
                                "tx"]

    @staticmethod
    def GetDescription():
        return "Texture files should be .jpg or .png."

    def __init__(self, verbose):
        # Check if the prim has an allowed type.
        super(TextureChecker, self).__init__(verbose)

    def _CheckTexture(self, texAssetPath):
        self._Msg("Checking texture <%s>." % texAssetPath)
        texFileExt = Ar.GetResolver().GetExtension(texAssetPath)
        if texFileExt in \
            TextureChecker._unsupportedImageFormats:
            self._AddFailedCheck("Found texture file '%s' with unsupported "
                    "file format." % texAssetPath)
        elif texFileExt not in \
            TextureChecker._allowedImageFormats:
            self._AddFailedCheck("Found texture file '%s' with unknown file "
                    "format." % texAssetPath)

    def CheckPrim(self, prim):
        # Right now, we find texture referenced by looking at the asset-valued 
        # shader inputs. However, it is entirely legal to feed the "fileName" 
        # input of a UsdUVTexture shader from a UsdPrimvarReader_string. 
        # Hence, ideally we would also check "the right" primvars on 
        # geometry prims here.  However, identifying the right primvars is 
        # non-trivial. We probably need to pre-analyze all the materials. 
        # Not going to try to do this yet, but it raises an interesting 
        # validation pattern -

        # Check if the prim is a shader. 
        if prim.GetTypeName() != "Shader":
            return

        from pxr import Sdf, UsdShade
        shader = UsdShade.Shader(prim)
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

class ARKitPackageEncapsulationChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "If the root layer is a package, then the composed stage "\
               "should not contain references to files outside the package. "\
               "In other words, the package should be entirely self-contained."

    def __init__(self, verbose):
        super(ARKitPackageEncapsulationChecker, self).__init__(verbose)

    def CheckDependencies(self, usdStage, layerDeps, assetDeps):
        rootLayer = usdStage.GetRootLayer()
        if not _IsPackageOrPackagedLayer(rootLayer):
            return

        packagePath = usdStage.GetRootLayer().realPath
        if packagePath:
            if Ar.IsPackageRelativePath(packagePath):
                packagePath = Ar.SplitPackageRelativePathOuter(
                        packagePath)[0]
            for layer in layerDeps:
                # In-memory layers like session layers (which we must skip when 
                # doing this check) won't have a real path.
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
    
class ARKitLayerChecker(BaseRuleChecker):
    # Only core USD file formats are allowed.
    _allowedLayerFormatIds = ('usd', 'usda', 'usdc', 'usdz')

    @staticmethod
    def GetDescription():
        return "All included layers that participate in composition should"\
            " have one of the core supported file formats."

    def __init__(self, verbose):
        # Check if the prim has an allowed type.
        super(ARKitLayerChecker, self).__init__(verbose)

    def CheckLayer(self, layer):
        self._Msg("Checking layer <%s>." % layer.identifier)

        formatId = layer.GetFileFormat().formatId
        if not formatId in \
            ARKitLayerChecker._allowedLayerFormatIds:
            self._AddFailedCheck("Layer '%s' has unsupported formatId "
                    "'%s'." % (layer.identifier, formatId))

class ARKitPrimTypeChecker(BaseRuleChecker):
    # All core prim types other than UsdGeomPointInstancers and the type in 
    # UsdLux are allowed.
    _allowedPrimTypeNames = ('', 'Scope', 'Xform', 'Camera',
                            'Shader', 'Material',
                            'Mesh', 'Sphere', 'Cube', 'Cylinder', 'Cone',
                            'Capsule', 'GeomSubset', 'Points', 
                            'SkelRoot', 'Skeleton', 'SkelAnimation', 
                            'BlendShape')

    @staticmethod
    def GetDescription():
        return "UsdGeomPointInstancers and custom schemas not provided by "\
                "core USD are not allowed."

    def __init__(self, verbose):
        # Check if the prim has an allowed type.
        super(ARKitPrimTypeChecker, self).__init__(verbose)

    def CheckPrim(self, prim):
        self._Msg("Checking prim <%s>." % prim.GetPath())
        if prim.GetTypeName() not in \
            ARKitPrimTypeChecker._allowedPrimTypeNames:
            self._AddFailedCheck("Prim <%s> has unsupported type '%s'." % 
                                    (prim.GetPath(), prim.GetTypeName()))

class ARKitStageYupChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "The stage and all fo the assets referenced within it "\
            "should be Y-up.",

    def __init__(self, verbose):
        # Check if the prim has an allowed type.
        super(ARKitStageYupChecker, self).__init__(verbose)

    def CheckStage(self, usdStage):
        from pxr import UsdGeom
        upAxis = UsdGeom.GetStageUpAxis(usdStage)
        if upAxis != UsdGeom.Tokens.y:
            self._AddFailedCheck("Stage has upAxis '%s'. upAxis should be "
                    "'%s'." % (upAxis, UsdGeom.Tokens.y))

class ARKitShaderChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "Shader nodes must have \"id\" as the implementationSource, "  \
               "with id values that begin with \"Usd*\". Also, shader inputs "\
               "with connections must each have a single, valid connection "  \
               "source."

    def __init__(self, verbose):
        super(ARKitShaderChecker, self).__init__(verbose)

    def CheckPrim(self, prim):
        from pxr import UsdShade

        if not prim.IsA(UsdShade.Shader):
            return

        shader = UsdShade.Shader(prim)
        if not shader:
            self._AddError("Invalid shader prim <%s>." % prim.GetPath())
            return

        self._Msg("Checking shader <%s>." % prim.GetPath())

        implSource = shader.GetImplementationSource()
        if implSource != UsdShade.Tokens.id:
            self._AddFailedCheck("Shader <%s> has non-id implementation "
                    "source '%s'." % (prim.GetPath(), implSource))

        shaderId = shader.GetShaderId()
        if not shaderId or \
           not (shaderId in ['UsdPreviewSurface', 'UsdUVTexture'] or
                shaderId.startswith('UsdPrimvarReader')) :
            self._AddFailedCheck("Shader <%s> has unsupported info:id '%s'." 
                    % (prim.GetPath(), shaderId))

        # Check shader input connections
        shaderInputs = shader.GetInputs()
        for shdInput in shaderInputs: 
            connections = shdInput.GetAttr().GetConnections()
            # If an input has one or more connections, ensure that the 
            # connections are valid.
            if len(connections) > 0:
                if len(connections) > 1:
                    self._AddFailedCheck("Shader input <%s> has %s connection "
                        "sources, but only one is allowed." % 
                        (shdInput.GetAttr.GetPath(), len(connections)))
                connectedSource = shdInput.GetConnectedSource()
                if connectedSource is None:
                    self._AddFailedCheck("Connection source <%s> for shader "
                        "input <%s> is missing." % (connections[0], 
                        shdInput.GetAttr().GetPath()))
                else:
                    # The source must be a valid shader or material prim.
                    source = connectedSource[0]
                    if not source.GetPrim().IsA(UsdShade.Shader) and \
                       not source.GetPrim().IsA(UsdShade.Material):
                        self._AddFailedCheck("Shader input <%s> has an invalid "
                            "connection source prim of type '%s'." % 
                            (shdInput.GetAttr().GetPath(), 
                             source.GetPrim().GetTypeName()))

class ARKitMaterialBindingChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "All material binding relationships must have valid targets."

    def __init__(self, verbose):
        super(ARKitMaterialBindingChecker, self).__init__(verbose)

    def CheckPrim(self, prim):
        from pxr import UsdShade
        relationships = prim.GetRelationships()
        bindingRels = [rel for rel in relationships if 
                rel.GetName().startswith(UsdShade.Tokens.materialBinding)]
        
        for bindingRel in bindingRels:
            targets = bindingRel.GetTargets()
            if len(targets) == 1:
                directBinding = UsdShade.MaterialBindingAPI.DirectBinding(
                        bindingRel)
                if not directBinding.GetMaterial():
                    self._AddFailedCheck("Direct material binding <%s> targets "
                        "an invalid material <%s>." % (bindingRel.GetPath(), 
                        directBinding.GetMaterialPath()))
            elif len(targets) == 2:
                collBinding = UsdShade.MaterialBindingAPI.CollectionBinding(
                        bindingRel)
                if not collBinding.GetMaterial():
                    self._AddFailedCheck("Collection-based material binding "
                        "<%s> targets an invalid material <%s>." % 
                        (bindingRel.GetPath(), collBinding.GetMaterialPath()))
                if not collBinding.GetCollection():
                    self._AddFailedCheck("Collection-based material binding "
                        "<%s> targets an invalid collection <%s>." % 
                        (bindingRel.GetPath(), collBinding.GetCollectionPath()))

class ARKitFileExtensionChecker(BaseRuleChecker):
    _allowedFileExtensions = \
        ARKitLayerChecker._allowedLayerFormatIds + \
        TextureChecker._allowedImageFormats

    @staticmethod
    def GetDescription():
        return "Only layer files and textures are allowed in a package."

    def __init__(self, verbose):
        super(ARKitFileExtensionChecker, self).__init__(verbose)

    def CheckZipFile(self, zipFile, packagePath):
        fileNames = zipFile.GetFileNames()
        for fileName in fileNames:
            fileExt = Ar.GetResolver().GetExtension(fileName)
            if fileExt not in ARKitFileExtensionChecker._allowedFileExtensions:
                self._AddFailedCheck("File '%s' in package '%s' has an "
                    "unknown or unsupported extension '%s'." % 
                    (fileName, packagePath, fileExt))

class ARKitRootLayerChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "The root layer of the package must be a usdc file and " \
            "must not include any external dependencies that participate in "\
            "stage composition."

    def __init__(self, verbose):
        super(ARKitRootLayerChecker, self).__init__(verbose=verbose)

    def CheckStage(self, usdStage):
        usedLayers = usdStage.GetUsedLayers()
        # This list excludes any session layers.
        usedLayersOnDisk = [i for i in usedLayers if i.realPath]
        if len(usedLayersOnDisk) > 1:
            self._AddFailedCheck("The stage uses %s layers. It should "
                "contain a single usdc layer to be compatible with ARKit's "
                "implementation of usdz." % len(usedLayersOnDisk))
        
        rootLayerRealPath = usdStage.GetRootLayer().realPath
        if rootLayerRealPath.endswith(".usdz"):
            # Check if the root layer in the package is a usdc.
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
                    rootLayerRealPath))
        elif not rootLayerRealPath.endswith(".usdc"):
            self._AddFailedCheck("Root layer of the stage '%s' does not "
                "have the '.usdc' extension." % (rootLayerRealPath))

class ComplianceChecker(object):
    """ A utility class for checking compliance of a given USD asset or a USDZ 
    package.

    Since usdz files are zip files, someone could use generic zip tools to 
    create an archive and just change the extension, producing a .usdz file that 
    does not honor the additional constraints that usdz files require.  Even if 
    someone does use our official archive creation tools, though, we 
    intentionally allow creation of usdz files that can be very "permissive" in 
    their contents for internal studio uses, where portability outside the 
    studio is not a concern.  For content meant to be delivered over the web 
    (eg. ARKit assets), however, we must be much more restrictive.

    This class provides two levels of compliance checking: 
    * "structural" validation that is represented by a set of base rules. 
    * "ARKit" compatibility validation, which includes many more restrictions.
    
    Calling ComplianceChecker.DumpAllRules() will print an enumeration of the 
    various rules in the two categories of compliance checking.
    """

    @staticmethod
    def GetBaseRules():
        return [ByteAlignmentChecker, CompressionChecker, 
                MissingReferenceChecker, TextureChecker]

    @staticmethod
    def GetARKitRules(skipARKitRootLayerCheck=False):
        arkitRules = [ARKitLayerChecker, ARKitPrimTypeChecker, 
                      ARKitStageYupChecker, ARKitShaderChecker, 
                      ARKitMaterialBindingChecker,
                      ARKitFileExtensionChecker, 
                      ARKitPackageEncapsulationChecker]
        if not skipARKitRootLayerCheck:
            arkitRules.append(ARKitRootLayerChecker)
        return arkitRules

    @staticmethod
    def GetRules(arkit=False, skipARKitRootLayerCheck=False):
        allRules = ComplianceChecker.GetBaseRules()
        if arkit:
            arkitRules = ComplianceChecker.GetARKitRules(
                    skipARKitRootLayerCheck=skipARKitRootLayerCheck)
            allRules += arkitRules
        return allRules

    @staticmethod 
    def DumpAllRules():
        print 'Base rules:'
        for ruleNum, rule in enumerate(GetBaseRules()):
            print '[%s] %s' % (ruleNum + 1, rule.GetDescription())
        print '-' * 30
        print 'ARKit rules: '
        for ruleNum, rule in enumerate(GetBaseRules()):
            print '[%s] %s' % (ruleNum + 1, rule.GetDescription())
        print '-' * 30
            
    def __init__(self, arkit=False, skipARKitRootLayerCheck=False,
                 rootPackageOnly=False, skipVariants=False, verbose=False):
        self._rootPackageOnly = rootPackageOnly
        self._doVariants = not skipVariants
        self._verbose = verbose
        self._errors = []

        # Once a package has been checked, it goes into this set. 
        self._checkedPackages = set()

        # Instantiate an instance of every rule checker and store in a list.
        self._rules = [Rule(self._verbose) for Rule in 
                ComplianceChecker.GetRules(arkit, skipARKitRootLayerCheck)]

    def _Msg(self, msg):
        if self._verbose:
            print msg
    
    def _AddError(self, errMsg):
        self._errors.append(errMsg)

    def GetErrors(self):
        errors = self._errors
        for rule in self._rules:
            errs = rule.GetErrors()
            for err in errs:
                errors.append("Error checking rule '%s': %s" % 
                    (type(rule).__name__, err))
        return errors

    def DumpRules(self):
        descriptions = [rule.GetDescription() for rule in self._rules]
        print 'Checking rules: '
        for ruleNum, rule in enumerate(descriptions):
            print '[%s] %s' % (ruleNum + 1, rule)
        print '-' * 30

    def GetFailedChecks(self):
        failedChecks = []
        for rule in self._rules:
            fcs = rule.GetFailedChecks()
            for fc in fcs:
                failedChecks.append("%s (fails '%s')" % (fc, 
                        type(rule).__name__))
        return failedChecks

    def CheckCompliance(self, inputFile):
        from pxr import Sdf, Usd, UsdUtils
        if not Usd.Stage.IsSupportedFile(inputFile):
            _AddError("Cannot open file '%s' on a USD stage." % args.inputFile)
            return

        # Collect all warnings using a diagnostic delegate.
        delegate = UsdUtils.CoalescingDiagnosticDelegate()
        usdStage = Usd.Stage.Open(inputFile)
        stageOpenDiagnostics = delegate.TakeUncoalescedDiagnostics()

        for rule in self._rules:
            rule.CheckStage(usdStage)
            rule.CheckDiagnostics(stageOpenDiagnostics)

        with Ar.ResolverContextBinder(usdStage.GetPathResolverContext()):
            # This recursively computes all of inputFiles's external 
            # dependencies.
            (allLayers, allAssets, unresolvedPaths) = \
                    UsdUtils.ComputeAllDependencies(Sdf.AssetPath(inputFile))
            for rule in self._rules:
                rule.CheckUnresolvedPaths(unresolvedPaths)
                rule.CheckDependencies(usdStage, allLayers, allAssets)

            if self._rootPackageOnly:
                rootLayer = usdStage.GetRootLayer()
                if rootLayer.GetFileFormat().IsPackage():
                    packagePath = Ar.SplitPackageRelativePathInner(
                            rootLayer.identifier)[0]
                    self._CheckPackage(packagePath)
                else:
                    self._AddError("Root layer of the USD stage (%s) doesn't belong to "
                        "a package, but 'rootPackageOnly' is True!" % 
                        Usd.Describe(usdStage))
            else:
                # Process every package just once by storing them all in a set.
                packages = set()
                for layer in allLayers:
                    if _IsPackageOrPackagedLayer(layer):
                        packagePath = Ar.SplitPackageRelativePathInner(
                                layer.identifier)[0]
                        packages.add(packagePath)
                    self._CheckLayer(layer)
                for package in packages:
                    self._CheckPackage(package)

                # Traverse the entire stage and check every prim.
                from pxr import Usd
                # Author all variant switches in the session layer.
                usdStage.SetEditTarget(usdStage.GetSessionLayer())
                allPrimsIt = iter(Usd.PrimRange.Stage(usdStage, 
                        Usd.TraverseInstanceProxies()))
                self._TraverseRange(allPrimsIt, isStageRoot=True)
    
    def _CheckPackage(self, packagePath):
        self._Msg("Checking package <%s>." % packagePath)

        # XXX: Should we open the package on a stage to ensure that it is valid 
        # and entirely self-contained.

        from pxr import Usd
        pkgExt = Ar.GetResolver().GetExtension(packagePath)
        if pkgExt != "usdz":
            self._AddError("Package at path %s has an invalid extension." 
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
            self._AddError("Failed to resolve package path '%s'." % packagePath)
            return

        zipFile = Usd.ZipFile.Open(resolvedPath)
        if not zipFile:
            self._AddError("Could not open package at path '%s'." % 
                           resolvedPath)
            return
        for rule in self._rules:
            rule.CheckZipFile(zipFile, packagePath)

    def _CheckLayer(self, layer):
        for rule in self._rules:
            rule.CheckLayer(layer)

    def _CheckPrim(self, prim):
        for rule in self._rules:
            rule.CheckPrim(prim)

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
