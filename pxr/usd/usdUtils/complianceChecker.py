#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function

import warnings

from pxr import Ar

from pxr.UsdUtils.constantsGroup import ConstantsGroup

class NodeTypes(ConstantsGroup):
    UsdPreviewSurface = "UsdPreviewSurface"
    UsdUVTexture = "UsdUVTexture"
    UsdTransform2d = "UsdTransform2d"
    UsdPrimvarReader = "UsdPrimvarReader"

class ShaderProps(ConstantsGroup):
    Bias = "bias"
    Scale = "scale"
    SourceColorSpace = "sourceColorSpace"
    Normal = "normal"
    File = "file"


def _IsPackageOrPackagedLayer(layer):
    return layer.GetFileFormat().IsPackage() or \
           Ar.IsPackageRelativePath(layer.identifier)

class BaseRuleChecker(object):
    """This is Base class for all the rule-checkers."""
    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        self._verbose = verbose
        self._consumerLevelChecks = consumerLevelChecks
        self._assetLevelChecks = assetLevelChecks
        self._failedChecks = []
        self._errors = []
        self._warnings = []

    def _AddFailedCheck(self, msg):
        self._failedChecks.append(msg)

    def _AddError(self, msg):
        self._errors.append(msg)

    def _AddWarning(self, msg):
        self._warnings.append(msg)

    def _Msg(self, msg):
        if self._verbose:
            print(msg)

    def GetFailedChecks(self):
        return self._failedChecks

    def GetErrors(self):
        return self._errors

    def GetWarnings(self):
        return self._warnings

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

    def ResetCaches(self):
        """ Reset any caches the rule owns.  Called whenever stage authoring
        occurs, such as when we iterate through VariantSet combinations.
        """
        pass

    # -------------------------------------------------------------------------

class ByteAlignmentChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "Files within a usdz package must be laid out properly, "\
                "i.e. they should be aligned to 64 bytes."

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(ByteAlignmentChecker, self).__init__(verbose,
                                                   consumerLevelChecks,
                                                   assetLevelChecks)

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
        return "Files within a usdz package should not be compressed or "\
               "encrypted."

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(CompressionChecker, self).__init__(verbose,
                                                 consumerLevelChecks,
                                                 assetLevelChecks)

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

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(MissingReferenceChecker, self).__init__(verbose, 
                                                      consumerLevelChecks,
                                                      assetLevelChecks)

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

class StageMetadataChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return """All stages should declare their 'upAxis' and 'metersPerUnit'.  
Stages that can be consumed as referencable assets should furthermore have
a valid 'defaultPrim' declared, and stages meant for consumer-level packaging
should always have upAxis set to 'Y'"""

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(StageMetadataChecker, self).__init__(verbose,
                                                   consumerLevelChecks,
                                                   assetLevelChecks)

    def CheckStage(self, usdStage):
        from pxr import UsdGeom

        if not usdStage.HasAuthoredMetadata(UsdGeom.Tokens.upAxis):
            self._AddFailedCheck("Stage does not specify an upAxis.")
        elif self._consumerLevelChecks:
            upAxis = UsdGeom.GetStageUpAxis(usdStage)
            if upAxis != UsdGeom.Tokens.y:
                self._AddFailedCheck("Stage specifies upAxis '%s'. upAxis should"
                                     " be '%s'." % (upAxis, UsdGeom.Tokens.y))
        
        if not usdStage.HasAuthoredMetadata(UsdGeom.Tokens.metersPerUnit):
            self._AddFailedCheck("Stage does not specify its linear scale "
                                 "in metersPerUnit.")

        if self._assetLevelChecks:
            defaultPrim = usdStage.GetDefaultPrim()
            if not defaultPrim:
                self._AddFailedCheck("Stage has missing or invalid defaultPrim.")
                
        

class TextureChecker(BaseRuleChecker):
    # The most basic formats are those published in the USDZ spec
    _basicUSDZImageFormats = ("exr", "jpg", "jpeg", "png")

    # Include a list of "unsupported" image formats to provide better error
    # messages when we find one of these.  Our builtin image decoder
    # _can_ decode these, but they are not considered portable consumer-level
    _unsupportedImageFormats = ["bmp", "tga", "hdr", "tif", "tx", "zfile"] 

    @staticmethod
    def GetDescription():
        return """Texture files should be readable by intended client 
(only .jpg, .jpeg or .png for consumer-level USDZ)."""

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        # Check if the prim has an allowed type.
        super(TextureChecker, self).__init__(verbose, consumerLevelChecks, 
                                             assetLevelChecks)
        # a None value for _allowedFormats indicates all formats are allowed
        self._allowedFormats = None

    def CheckStage(self, usdStage):
        # This is the point at which we can determine whether we have a USDZ
        # archive, and so have enough information to set the allowed formats.
        rootLayer = usdStage.GetRootLayer()
        if rootLayer.GetFileFormat().IsPackage() or self._consumerLevelChecks:
            self._allowedFormats = list(TextureChecker._basicUSDZImageFormats)
        else:
            self._Msg("Not performing texture format checks for general "
                      "USD asset")


    def _CheckTexture(self, texAssetPath, inputPath):
        self._Msg("Checking texture <%s>." % texAssetPath)
        texFileExt = Ar.GetResolver().GetExtension(texAssetPath).lower()
        if (self._consumerLevelChecks and
            texFileExt in TextureChecker._unsupportedImageFormats):
            self._AddFailedCheck("Texture <%s> with asset @%s@ has non-portable "
                                 "file format." % (inputPath, texAssetPath))
        elif texFileExt not in self._allowedFormats:
            self._AddFailedCheck("Texture <%s> with asset @%s@ has unknown "
                                 "file format." % (inputPath, texAssetPath))

    def CheckPrim(self, prim):
        # Right now, we find texture referenced by looking at the asset-valued 
        # inputs on Connectable prims. 
        from pxr import Sdf, Usd, UsdShade

        # Nothing to do if we are allowing all formats, or if 
        # we are an untyped prim
        if self._allowedFormats is None or not prim.GetTypeName():
            return
            
        # Check if the prim is Connectable.
        connectable = UsdShade.ConnectableAPI(prim)
        if not connectable:
            return

        shaderInputs = connectable.GetInputs()
        for ip in shaderInputs:
            attrPath = ip.GetAttr().GetPath()
            if ip.GetTypeName() == Sdf.ValueTypeNames.Asset:
                texFilePath = ip.Get(Usd.TimeCode.EarliestTime())
                # ip may be unauthored and/or connected
                if texFilePath:
                    self._CheckTexture(texFilePath.path, attrPath)
            elif ip.GetTypeName() == Sdf.ValueTypeNames.AssetArray:
                texPathArray = ip.Get(Usd.TimeCode.EarliestTime())
                if texPathArray:
                    for texPath in texPathArray:
                        self._CheckTexture(texPath, attrPath)

class PrimEncapsulationChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return """Check for basic prim encapsulation rules:
   - Boundables may not be nested under Gprims
   - Connectable prims (e.g. Shader, Material, etc) can only be nested 
     inside other Container-like Connectable prims. Container-like prims
     include Material, NodeGraph, Light, LightFilter, and *exclude Shader*"""

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(PrimEncapsulationChecker, self).__init__(verbose, 
                                                       consumerLevelChecks,
                                                       assetLevelChecks)
        self.ResetCaches()

    def _HasGprimAncestor(self, prim):
        from pxr import Sdf, UsdGeom
        path = prim.GetPath()
        if path in self._hasGprimInPathMap:
            return self._hasGprimInPathMap[path]
        elif path == Sdf.Path.absoluteRootPath:
            self._hasGprimInPathMap[path] = False
            return False
        else:
            val = (self._HasGprimAncestor(prim.GetParent()) or 
                   prim.IsA(UsdGeom.Gprim))
            self._hasGprimInPathMap[path] = val
            return val
        
    def _FindConnectableAncestor(self, prim):
        from pxr import Sdf, UsdShade
        path = prim.GetPath()
        if path in self._connectableAncestorMap:
            return self._connectableAncestorMap[path]
        elif path == Sdf.Path.absoluteRootPath:
            self._connectableAncestorMap[path] = None
            return None
        else:
            val = self._FindConnectableAncestor(prim.GetParent())
            # The GetTypeName() check is to work around a bug in
            # ConnectableAPIBehavior registry.
            if prim.GetTypeName() and not val:
                conn = UsdShade.ConnectableAPI(prim)
                if conn:
                    val = prim
            self._connectableAncestorMap[path] = val
            return val
        
            
    def CheckPrim(self, prim):
        from pxr import UsdGeom, UsdShade
        
        parent = prim.GetParent()

        # Of course we must allow Boundables under other Boundables, so that
        # schemas like UsdGeom.Pointinstancer can nest their prototypes.  But
        # we disallow a PointInstancer under a Mesh just as we disallow a Mesh 
        # under a Mesh, for the same reason: we cannot then independently
        # adjust visibility for the two objects, nor can we reasonably compute
        # the parent Mesh's extent.
        if prim.IsA(UsdGeom.Boundable):
            if parent:
                if self._HasGprimAncestor(parent):
                    self._AddFailedCheck("Gprim <%s> has an ancestor prim that "
                                         "is also a Gprim, which is not allowed."
                                         % prim.GetPath())
                    
        connectable = UsdShade.ConnectableAPI(prim)
        # The GetTypeName() check is to work around a bug in
        # ConnectableAPIBehavior registry.
        if prim.GetTypeName() and connectable:
            if parent:
                pConnectable = UsdShade.ConnectableAPI(parent)
                if not parent.GetTypeName():
                    pConnectable = None
                if pConnectable and not pConnectable.IsContainer():
                    # It is a violation of the UsdShade OM which enforces
                    # encapsulation of connectable prims under a Container-type
                    # connectable prim.
                    self._AddFailedCheck("Connectable %s <%s> cannot reside "
                                     "under a non-Container Connectable %s"
                                     % (prim.GetTypeName(),
                                        prim.GetPath(),
                                        parent.GetTypeName()))
                elif not pConnectable:
                    # it's only OK to have a non-connectable parent if all
                    # the rest of your ancestors are also non-connectable.  The
                    # error message we give is targeted at the most common
                    # infraction, using Scope or other grouping prims inside
                    # a Container like a Material
                    connAnstr = self._FindConnectableAncestor(parent)
                    if connAnstr is not None:
                        self._AddFailedCheck("Connectable %s <%s> can only have"
                                             " Connectable Container ancestors"
                                             " up to %s ancestor <%s>, but its"
                                             " parent %s is a %s." %
                                             (prim.GetTypeName(),
                                              prim.GetPath(),
                                              connAnstr.GetTypeName(),
                                              connAnstr.GetPath(),
                                              parent.GetName(),
                                              parent.GetTypeName()))
                

    def ResetCaches(self):
        self._connectableAncestorMap = dict()
        self._hasGprimInPathMap = dict()



class NormalMapTextureChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return """UsdUVTexture nodes that feed the _inputs:normals_ of a
UsdPreviewSurface must ensure that the data is encoded and scaled properly.
Specifically:
   - Since normals are expected to be in the range [(-1,-1,-1), (1,1,1)],
     the Texture node must transform 8-bit textures from their [0..1] range by
     setting its _inputs:scale_ to (2, 2, 2, 1) and 
     _inputs:bias_ to (-1, -1, -1, 0)
   - Normal map data is commonly expected to be linearly encoded.  However, many
     image-writing tools automatically set the profile of three-channel, 8-bit
     images to SRGB.  To prevent an unwanted transformation, the UsdUVTexture's
     _inputs:sourceColorSpace_ must be set to "raw".
"""
    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(NormalMapTextureChecker, self).__init__(verbose, 
                                                      consumerLevelChecks, 
                                                      assetLevelChecks)

    def _GetShaderId(self, shader):
        # We might someday try harder to find an identifier...
        return shader.GetShaderId()

    def _TextureIs8Bit(self, asset):
        # Eventually we hope to leverage HioImage through a plugin system,
        # when Imaging is present, to answer this and other image queries
        # more definitively
        from pxr import Ar
        ext = Ar.GetResolver().GetExtension(asset.resolvedPath)
        # not an exhaustive list, but ones we typically can read
        return ext in ["bmp", "tga", "jpg", "jpeg", "png", "tif"]

    def _GetInputValue(self, shader, inputName):
        from pxr import Usd, UsdShade
        input = shader.GetInput(inputName)
        if not input:
            return None
        # Query value producing attributes for input values.
        # This has to be a length of 1, otherwise no attribute is producing a value.
        valueProducingAttrs = UsdShade.Utils.GetValueProducingAttributes(input)
        if not valueProducingAttrs or len(valueProducingAttrs) != 1:
            return None
        # We require an input parameter producing the value.
        if not UsdShade.Input.IsInput(valueProducingAttrs[0]):
            return None
        return valueProducingAttrs[0].Get(Usd.TimeCode.EarliestTime())

    def CheckPrim(self, prim):
        from pxr import UsdShade, Gf
        from pxr.UsdShade import Utils as ShadeUtils

        if not prim.IsA(UsdShade.Shader):
            return

        shader = UsdShade.Shader(prim)
        if not shader:
            self._AddError("Invalid shader prim <%s>." % prim.GetPath())
            return

        shaderId = self._GetShaderId(shader)

        # We may have failed to fetch an identifier for asset/source-based
        # nodes. We are only interested in UsdPreviewSurface nodes identified via
        # info:id, so it's not an error
        if not shaderId or shaderId != NodeTypes.UsdPreviewSurface:
            return

        normalInput = shader.GetInput(ShaderProps.Normal)
        if not normalInput:
            return
        valueProducingAttrs = ShadeUtils.GetValueProducingAttributes(normalInput)
        if not valueProducingAttrs or valueProducingAttrs[0].GetPrim() == prim:
            return

        sourcePrim = valueProducingAttrs[0].GetPrim()

        sourceShader = UsdShade.Shader(sourcePrim)
        if not sourceShader:
            # In theory, could be connected to an interface attribute of a
            # parent connectable... not useful, but not an error
            if UsdShade.ConnectableAPI(sourcePrim):
                return
            self._AddFailedCheck("%s.%s on prim <%s> is connected to a"
                                 " non-Shader prim." % \
                                 (NodeTypes.UsdPreviewSurface,
                                  ShaderProps.Normal))
            return
        
        sourceId = self._GetShaderId(sourceShader)

        # We may have failed to fetch an identifier for asset/source-based
        # nodes. OR, we could potentially be driven by a UsdPrimvarReader,
        # in which case we'd have nothing to validate
        if not sourceId or sourceId != NodeTypes.UsdUVTexture:
            return
           
        texAsset = self._GetInputValue(sourceShader, ShaderProps.File)
        
        if not texAsset or not texAsset.resolvedPath:
            self._AddFailedCheck("%s prim <%s> has invalid or unresolvable "
                                 "inputs:file of @%s@" % \
                                 (NodeTypes.UsdUVTexture,
                                  sourcePrim.GetPath(),
                                  texAsset.path if texAsset else ""))
            return
            
        if not self._TextureIs8Bit(texAsset):
            # really nothing more is required for image depths > 8 bits,
            # which we assume FOR NOW, are floating point
            return

        # -- 8-bit texture validations --
        colorSpace = self._GetInputValue(sourceShader,
                ShaderProps.SourceColorSpace)
        if not colorSpace or not colorSpace == 'raw':
            self._AddError("%s prim <%s> that reads Normal Map @%s@ should "
                             "set inputs:sourceColorSpace to 'raw'." % \
                             (NodeTypes.UsdUVTexture,
                              sourcePrim.GetPath(),
                              texAsset.path))

        bias = self._GetInputValue(sourceShader, ShaderProps.Bias)
 
        scale = self._GetInputValue(sourceShader, ShaderProps.Scale)

        if not (bias and scale and 
                isinstance(bias, Gf.Vec4f) and isinstance(scale, Gf.Vec4f)):
            self._AddError("%s prim <%s> reads 8 bit Normal Map @%s@, "
                           "which requires that inputs:scale be set to "
                           "(2, 2, 2, 1) and inputs:bias be set to "
                           "(-1, -1, -1, 0) for proper interpretation as per "
                           "the UsdPreviewSurface and UsdUVTexture docs." %\
                             (NodeTypes.UsdUVTexture,
                              sourcePrim.GetPath(),
                              texAsset.path))
            return

        # We still warn for inputs:scale not conforming to UsdPreviewSurface
        # guidelines, as some authoring tools may rely on this to scale an
        # effect of normal perturbations.
        # don't really care about fourth components...
        nonCompliantScaleValues = \
                (scale[0] != 2 or scale[1] != 2 or scale[2] != 2)
        if nonCompliantScaleValues:
            self._AddWarning("%s prim <%s> reads an 8 bit Normal Map, "
                             "but has non-standard inputs:scale value of %s." 
                             "inputs:scale must be set to (2, 2, 2, 1) so as " 
                             "fullfill the requirements of the normals to be " 
                             "in tangent space of [(-1,-1,-1), (1,1,1)] as "
                             "documented in the UsdPreviewSurface and "
                             "UsdUVTexture docs." %\
                             (NodeTypes.UsdUVTexture,
                              sourcePrim.GetPath(), str(scale)))

        

        # Note that for a 8bit normal map, inputs:bias must be appropriately
        # set to [-1, -1, -1, 0] so as to fullfill the requirements of the
        # normals to be in tangent space of [(-1,-1,-1), (1,1,1)] as documented 
        # in the UsdPreviewSurface docs. Note this is true only when scale
        # values are respecting the requirements laid in the
        # UsdPreviewSurface / UsdUVTexture docs. We continue to warn!
        if (not nonCompliantScaleValues and 
                (bias[0] != -1 or bias[1] != -1 or bias[2] != -1)):
            self._AddError("%s prim <%s> reads an 8 bit Normal Map, but has "
                           "non-standard inputs:bias value of %s. inputs:bias "
                           "must be set to [-1,-1,-1,0] so as to fullfill "
                           "the requirements of the normals to be in tangent "
                           "space of [(-1,-1,-1), (1,1,1)] as documented "
                           "in the UsdPreviewSurface and UsdUVTexture docs." %\
                             (NodeTypes.UsdUVTexture,
                              sourcePrim.GetPath(), str(bias)))

class MaterialBindingAPIAppliedChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "A prim providing a material binding, must have " \
               "MaterialBindingAPI applied on the prim."

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(MaterialBindingAPIAppliedChecker, self).__init__(verbose, 
                                                          consumerLevelChecks,
                                                          assetLevelChecks)

    def CheckPrim(self, prim):
        from pxr import UsdShade
        numMaterialBindings = len([rel for rel in prim.GetRelationships() \
                if rel.GetName().startswith(UsdShade.Tokens.materialBinding)])
        if ( (numMaterialBindings > 0) and 
            not prim.HasAPI(UsdShade.MaterialBindingAPI)):
                self._AddFailedCheck("Found material bindings but no " \
                    "MaterialBindingAPI applied on the prim <%s>." \
                    % prim.GetPath())

class SkelBindingAPIAppliedChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "A prim providing skelBinding properties, must have " \
               "SkelBindingAPI applied on the prim."

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        from pxr import Usd
        usdSchemaRegistry = Usd.SchemaRegistry()
        primDef = usdSchemaRegistry.BuildComposedPrimDefinition("",
                ["SkelBindingAPI"])
        self._skelBindingAPIProps = primDef.GetPropertyNames()
        super(SkelBindingAPIAppliedChecker, self).__init__(verbose,
                consumerLevelChecks, assetLevelChecks)

    def CheckPrim(self, prim):
        from pxr import UsdSkel
        if not prim.HasAPI(UsdSkel.BindingAPI):
            primProperties = prim.GetPropertyNames()
            for skelProperty in self._skelBindingAPIProps:
                if skelProperty in primProperties:
                    self._AddFailedCheck("Found a UsdSkelBinding property " \
                        "(%s) , but no SkelBindingAPI applied on the prim " \
                        "<%s>." %(skelProperty, prim.GetPath()))
                    return
        else:
            # If the API is already applied make sure this prim is either
            # SkelRoot type or is rooted under a SkelRoot prim, else prim won't
            # be considered for any UsdSkel Skinning.
            if prim.GetTypeName() == UsdSkel.Tokens.SkelRoot:
                return
            parentPrim = prim.GetParent()
            while not parentPrim.IsPseudoRoot():
                if parentPrim.GetTypeName() == UsdSkel.Tokens.SkelRoot:
                    return
                parentPrim = parentPrim.GetParent()
            self._AddFailedCheck("UsdSkelBindingAPI applied on a prim, which " \
                    "is not of type SkelRoot or is not rooted at a prim of " \
                    "type SkelRoot, as required by the UsdSkel schema.");

class ShaderPropertyTypeConformanceChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "Shader prim's input types must be conforming to their " \
            "appropriate Sdf types in the respective sdr shader."

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(ShaderPropertyTypeConformanceChecker, self).__init__(verbose,
                                                   consumerLevelChecks,
                                                   assetLevelChecks)

    def _FillSdrNameToTypeMap(self, shadeNode, mapping):
        for inputName in shadeNode.GetInputNames():
            prop = shadeNode.GetInput(inputName)
            propName = prop.GetName()
            mapping[propName] = prop.GetTypeAsSdfType().GetSdfType()



    def CheckPrim(self, prim):
        from pxr import Sdr, UsdShade

        if not prim.IsA(UsdShade.Shader):
            return

        shader = UsdShade.Shader(prim)
        if not shader:
            # Error has already been issued by a Base-level checker
            return

        self._Msg("Checking shader <%s>." % prim.GetPath())

        # Retrieve ground truth Sdf types for input properties for all source 
        # types
        sdrNameToTypeMapping = {}
        sdrShaderNode = None
        inputs = []
        expectedImplSources = [UsdShade.Tokens.id, 
                               UsdShade.Tokens.sourceAsset, 
                               UsdShade.Tokens.sourceCode]
        implSource = shader.GetImplementationSource()
        if implSource in expectedImplSources:
            sourceTypes = shader.GetSourceTypes()
            hasNoSources = not len(sourceTypes)
            if hasNoSources and implSource == UsdShade.Tokens.id:
                shaderId = shader.GetShaderId( )
                if not shaderId:
                    return
                sdrShaderNode = \
                    Sdr.Registry().GetShaderNodeByIdentifier(shaderId)
            elif hasNoSources:
                self._AddFailedCheck("Shader <%s> has no sourceType. "
                     % (prim.GetPath()))
                return
            else:
                # If a shader has multiple source types, they will all share
                # the same inputs so only one shaderNode needs to be checked
                sdrShaderNode = \
                    shader.GetShaderNodeForSourceType(sourceTypes[0])
        else:
            self._AddFailedCheck("Shader <%s> has invalid implementation "
                    "source '%s'." % (prim.GetPath(), implSource))
            return

        if not sdrShaderNode:
            self._AddFailedCheck("Shader <%s> has invalid shader node. "
                     % (prim.GetPath()))
            return

        self._FillSdrNameToTypeMap(sdrShaderNode, sdrNameToTypeMapping)

        # Compare schema properties to the ground truth Sdf values
        inputs = shader.GetInputs()
        for input in inputs:
            inputName = input.GetBaseName()
            schemaSdfType = input.GetTypeName()
            if inputName in sdrNameToTypeMapping:
                sdrSdfType = sdrNameToTypeMapping[inputName]
                if not (sdrSdfType == schemaSdfType):
                    self._AddFailedCheck("Incorrect type for %s. Expected '%s'"
                        "; got '%s'." %
                        (input.GetAttr().GetPath(), sdrSdfType, schemaSdfType))

class ARKitPackageEncapsulationChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "If the root layer is a package, then the composed stage "\
               "should not contain references to files outside the package. "\
               "In other words, the package should be entirely self-contained."

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(ARKitPackageEncapsulationChecker, self).\
            __init__(verbose, consumerLevelChecks, assetLevelChecks)

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

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        # Check if the prim has an allowed type.
        super(ARKitLayerChecker, self).__init__(verbose, 
                                                consumerLevelChecks,
                                                assetLevelChecks)

    def CheckLayer(self, layer):
        self._Msg("Checking layer <%s>." % layer.identifier)

        formatId = layer.GetFileFormat().formatId
        if not formatId in \
            ARKitLayerChecker._allowedLayerFormatIds:
            self._AddFailedCheck("Layer '%s' has unsupported formatId "
                    "'%s'." % (layer.identifier, formatId))

class ARKitPrimTypeChecker(BaseRuleChecker):
    # All core prim types other than UsdGeomPointInstancers, Curve types, Nurbs,
    # and the types in  UsdLux are allowed.
    _allowedPrimTypeNames = ('', 'Scope', 'Xform', 'Camera',
                            'Shader', 'Material',
                            'Mesh', 'Sphere', 'Cube', 'Cylinder', 'Cone',
                            'Capsule', 'GeomSubset', 'Points', 
                            'SkelRoot', 'Skeleton', 'SkelAnimation', 
                            'BlendShape', 'SpatialAudio', 'PhysicsScene',
                            'Preliminary_ReferenceImage', 'Preliminary_Text',
                            'Preliminary_Trigger')

    @staticmethod
    def GetDescription():
        return "UsdGeomPointInstancers and custom schemas not provided by "\
                "core USD are not allowed."

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        # Check if the prim has an allowed type.
        super(ARKitPrimTypeChecker, self).__init__(verbose, 
                                                   consumerLevelChecks,
                                                   assetLevelChecks)

    def CheckPrim(self, prim):
        self._Msg("Checking prim <%s>." % prim.GetPath())
        if (
            (prim.GetTypeName() not in ARKitPrimTypeChecker._allowedPrimTypeNames) and
            (not prim.GetTypeName().startswith("RealityKit"))
        ):
            self._AddFailedCheck("Prim <%s> has unsupported type '%s'." % 
                                    (prim.GetPath(), prim.GetTypeName()))

class ARKitShaderChecker(BaseRuleChecker):
    @staticmethod
    def GetDescription():
        return "Shader nodes must have \"id\" as the implementationSource, "  \
               "with id values that begin with \"Usd*|ND_*\". Also, shader inputs "\
               "with connections must each have a single, valid connection "  \
               "source."

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(ARKitShaderChecker, self).__init__(verbose, consumerLevelChecks, 
                                                 assetLevelChecks)

    def CheckPrim(self, prim):
        from pxr import UsdShade

        if not prim.IsA(UsdShade.Shader):
            return

        shader = UsdShade.Shader(prim)
        if not shader:
            # Error has already been issued by a Base-level checker
            return

        self._Msg("Checking shader <%s>." % prim.GetPath())

        implSource = shader.GetImplementationSource()
        if implSource != UsdShade.Tokens.id:
            self._AddFailedCheck("Shader <%s> has non-id implementation "
                    "source '%s'." % (prim.GetPath(), implSource))

        shaderId = shader.GetShaderId()
        if not shaderId or \
           not (shaderId in [NodeTypes.UsdPreviewSurface, 
                             NodeTypes.UsdUVTexture, 
                             NodeTypes.UsdTransform2d] or
                shaderId.startswith(NodeTypes.UsdPrimvarReader) or
                shaderId.startswith("ND_")) :
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

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(ARKitMaterialBindingChecker, self).__init__(verbose, 
                                                          consumerLevelChecks,
                                                          assetLevelChecks)

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
        TextureChecker._basicUSDZImageFormats

    @staticmethod
    def GetDescription():
        return "Only layer files and textures are allowed in a package."

    def __init__(self, verbose, consumerLevelChecks, assetLevelChecks):
        super(ARKitFileExtensionChecker, self).__init__(verbose, 
                                                        consumerLevelChecks, 
                                                        assetLevelChecks)

    def CheckZipFile(self, zipFile, packagePath):
        fileNames = zipFile.GetFileNames()
        for fileName in fileNames:
            fileExt = Ar.GetResolver().GetExtension(fileName)
            if fileExt not in ARKitFileExtensionChecker._allowedFileExtensions:
                self._AddFailedCheck("File '%s' in package '%s' has an "
                    "unknown or unsupported extension '%s'." % 
                    (fileName, packagePath, fileExt))


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
        return [ByteAlignmentChecker, CompressionChecker, ShaderPropertyTypeConformanceChecker,
                MissingReferenceChecker, StageMetadataChecker, TextureChecker, 
                PrimEncapsulationChecker, NormalMapTextureChecker,
                MaterialBindingAPIAppliedChecker, SkelBindingAPIAppliedChecker]

    @staticmethod
    def GetARKitRules(skipARKitRootLayerCheck=False):
        arkitRules = [ARKitLayerChecker, ARKitPrimTypeChecker, 
                      ARKitShaderChecker, 
                      ARKitMaterialBindingChecker,
                      ARKitFileExtensionChecker, 
                      ARKitPackageEncapsulationChecker]
        if skipARKitRootLayerCheck:
            warnings.warn("skipARKitRootLayerCheck is no longer supported. It will be removed in a future version",
                          PendingDeprecationWarning)
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
        print('Base rules:')
        for ruleNum, rule in enumerate(GetBaseRules()):
            print('[%s] %s' % (ruleNum + 1, rule.GetDescription()))
        print('-' * 30)
        print('ARKit rules: ')
        for ruleNum, rule in enumerate(GetBaseRules()):
            print('[%s] %s' % (ruleNum + 1, rule.GetDescription()))
        print('-' * 30)
            
    def __init__(self, arkit=False, skipARKitRootLayerCheck=False,
                 rootPackageOnly=False, skipVariants=False, verbose=False,
                 assetLevelChecks=True):
        self._rootPackageOnly = rootPackageOnly
        self._doVariants = not skipVariants
        self._verbose = verbose
        self._errors = []
        self._warnings = []

        # Once a package has been checked, it goes into this set. 
        self._checkedPackages = set()

        # Instantiate an instance of every rule checker and store in a list.
        self._rules = [Rule(self._verbose, arkit, assetLevelChecks) for Rule in 
                ComplianceChecker.GetRules(arkit, skipARKitRootLayerCheck)]

    def _Msg(self, msg):
        if self._verbose:
            print(msg)
    
    def _AddError(self, errMsg):
        self._errors.append(errMsg)

    def _AddWarning(self, errMsg):
        self._warnings.append(errMsg)

    def GetErrors(self):
        errors = self._errors
        for rule in self._rules:
            errs = rule.GetErrors()
            for err in errs:
                errors.append("Error checking rule '%s': %s" % 
                    (type(rule).__name__, err))
        return errors

    def GetWarnings(self):
        warnings = self._warnings
        for rule in self._rules:
            advisories = rule.GetWarnings()
            for ad in advisories:
                warnings.append("%s (may violate '%s')" % (ad, 
                        type(rule).__name__))
        return warnings

    def DumpRules(self):
        print('Checking rules: ')
        for rule in self._rules:
            print('-' * 30)
            print('[%s]:\n %s' % (type(rule).__name__, rule.GetDescription()))
        print('-' * 30)

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
            self._AddError("Cannot open file '%s' on a USD stage." % inputFile)
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
        if not resolvedPath:
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
            for rule in self._rules:
                rule.ResetCaches()
            primRangeIt = iter(Usd.PrimRange(prim, 
                    Usd.TraverseInstanceProxies()))
            self._TraverseRange(primRangeIt, isStageRoot=False)
