// Copyright 2023 Apple
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//

#include "complianceChecker.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <set>

#include "pxr/pxr.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ar/resolverContextBinder.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/fileFormat.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usd/variantSets.h"
#include "pxr/usd/usd/zipFile.h"
#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/metrics.h"
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"
#include "pxr/usd/usdShade/utils.h"
#include "pxr/usd/usdSkel/binding.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdUtils/coalescingDiagnosticDelegate.h"
#include "pxr/usd/usdUtils/dependencies.h"


PXR_NAMESPACE_OPEN_SCOPE

/// Recurse through each sequence, adding them to sequence stacks
// NOLINTNEXTLINE
void RecursiveCartesian(std::vector<std::vector<std::string>> &collector,
                        std::vector<std::string> &stack,
                        std::vector<std::vector<std::string>> sequences,
                        int currentIndex) {
    std::vector<std::string> sequence = sequences[currentIndex];
    for (const auto &item: sequence) {
        stack.emplace_back(item);
        if (!currentIndex) {
            collector.push_back(stack);
        } else {
            RecursiveCartesian(collector, stack, sequences,
                    // Recurse backwards because its more performant
                               currentIndex - 1);

        }
        stack.pop_back();
    }
}

/// Creates groupings of all combinations of the sequences
/// similar to itertools.product in Python
std::vector<std::vector<std::string>> CartesianProduct(
        const std::vector<std::vector<std::string>> &sequences) {
    if (sequences.empty()) {
        return {};
    }

    std::vector<std::vector<std::string>> groups;
    std::vector<std::string> stack;
    // Recurse backwards because it's easier to pop stuff the end
    RecursiveCartesian(groups, stack, sequences, static_cast<int>(sequences.size() - 1));

    // At the end, each of our sequences will be backwards
    // and so need to be reversed before finally returning
    for (auto &group: groups) {
        auto seq = group;
        reverse(seq.begin(), seq.end());
        group = seq;
    }
    return groups;
}

UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker(bool verbose,
                                                 bool consumerLevelChecks,
                                                 bool assetLevelChecks) :
        _verbose(verbose),
        _consumerLevelChecks(consumerLevelChecks),
        _assetLevelChecks(assetLevelChecks) {

}

void UsdUtilsBaseRuleChecker::Msg(const std::string &msg) const {
    if (_verbose) {
        std::cout << msg << std::endl;
    }
}

void UsdUtilsByteAlignmentChecker::CheckZipFile(const UsdZipFile &zipFile, const std::string &packagePath) {
    for (auto file = zipFile.begin(); file != zipFile.end(); ++file) {
        auto fileInfo = file.GetFileInfo();
        auto offset = fileInfo.dataOffset;
        if (offset % 64) {
            _failedChecks.emplace_back(
                    TfStringPrintf("File '%s' in package '%s' has an invalid offset %zu.",
                                   file->c_str(), packagePath.c_str(), offset)
            );
        }
    }
}

void UsdUtilsCompressionChecker::CheckZipFile(const UsdZipFile &zipFile, const std::string &packagePath) {
    for (auto file = zipFile.begin(); file != zipFile.end(); ++file) {
        auto fileInfo = file.GetFileInfo();
        if (fileInfo.compressionMethod) {
            _failedChecks.emplace_back(
                    TfStringPrintf(
                            "File '%s' in package '%s has compression. Compression method "
                            "is '%hu', actual size is %zu. Uncompressed size is %zu.",
                            file->c_str(), packagePath.c_str(), fileInfo.compressionMethod,
                            fileInfo.size, fileInfo.uncompressedSize)
            );
        }
    }
}

void UsdUtilsMissingReferenceChecker::CheckDiagnostics(
        const std::vector<std::unique_ptr<TfDiagnosticBase>> &diagnostics) {
    for (const auto &diagnostic: diagnostics) {
        if (diagnostic->GetSourceFunction().find("_ReportErrors") != std::string::npos &&
            diagnostic->GetSourceFileName().find("usd/stage.cpp") != std::string::npos) {
            _failedChecks.emplace_back(diagnostic->GetCommentary());
        }
    }
}

void UsdUtilsMissingReferenceChecker::CheckUnresolvedPaths(const std::vector<std::string> &unresolvedPaths) {
    for (const auto &unresolvedPath: unresolvedPaths) {
        _failedChecks.emplace_back(
                TfStringPrintf("Found unresolvable external dependency '%s'.", unresolvedPath.c_str()));
    }
}


void UsdUtilsStageMetadataChecker::CheckStage(const UsdStageRefPtr &stage) {
    if (!stage->HasAuthoredMetadata(UsdGeomTokens->upAxis)) {
        _failedChecks.emplace_back("Stage does not specify an upAxis.");
    } else if (_consumerLevelChecks) {
        auto upAxis = UsdGeomGetStageUpAxis(stage);
        if (upAxis != UsdGeomTokens->y) {
            _failedChecks.emplace_back(TfStringPrintf(
                    "Stage specifies upAxis '%s'."
                    "upAxis should be '%s'",
                    upAxis.data(), UsdGeomTokens->y.data()
            ));
        }
    }


    if (!stage->HasAuthoredMetadata(UsdGeomTokens->metersPerUnit)) {
        _failedChecks.emplace_back("Stage does not specify its linear scale in metersPerUnit.");
    }

    if (_assetLevelChecks) {
        auto defaultPrim = stage->GetDefaultPrim();
        if (!defaultPrim) {
            _failedChecks.emplace_back("Stage has missing or invalid defaultPrim.");
        }
    }
}

void UsdUtilsTextureChecker::CheckStage(const UsdStageRefPtr &stage) {
    auto rootLayer = stage->GetRootLayer();
    if (rootLayer->GetFileFormat()->IsPackage() || _consumerLevelChecks) {
        _checkBaseUSDZFiles = true;
    } else {
        Msg("Not performing texture format checks for general USD asset.");
    }
}

void UsdUtilsTextureChecker::CheckPrim(const UsdPrim &prim) {
    if (!_checkBaseUSDZFiles || prim.GetTypeName().IsEmpty()) {
        return;
    }

    auto connectable = UsdShadeConnectableAPI(prim);
    if (!connectable) {
        return;
    }

    auto shaderInputs = connectable.GetInputs();
    for (const auto &ip: shaderInputs) {
        auto attrPath = ip.GetAttr().GetPath();
        if (ip.GetTypeName() == SdfValueTypeNames->Asset) {
            SdfAssetPath texFilePath;
            if (ip.Get(&texFilePath, UsdTimeCode::EarliestTime())) {
                const auto &resolvedTexPath = texFilePath.GetResolvedPath();
                if (!resolvedTexPath.empty()) {
                    CheckTexture(resolvedTexPath, attrPath);
                }
            }
        } else if (ip.GetTypeName() == SdfValueTypeNames->AssetArray) {
            VtArray<SdfAssetPath> texPathArray;
            if (ip.Get(&texPathArray, UsdTimeCode::EarliestTime())) {
                for (const auto &texFilePath: texPathArray) {
                    auto resolvedTexPath = texFilePath.GetResolvedPath();
                    if (!resolvedTexPath.empty()) {
                        CheckTexture(resolvedTexPath, attrPath);
                    }
                }
            }
        }
    }
}

void UsdUtilsTextureChecker::CheckTexture(const std::string &texAssetPath,
                                          const SdfPath &inputPath) {
    Msg(TfStringPrintf("Checking texture <%s>", texAssetPath.c_str()));
    auto texFileExt = ArGetResolver().GetExtension(texAssetPath);
    texFileExt = TfStringToLower(texFileExt);

    if (_consumerLevelChecks && _unsupportedImageFormats.find(texFileExt) != _unsupportedImageFormats.end()) {
        _failedChecks.emplace_back(TfStringPrintf(
                "Texture <%s> with asset @%s@ has non-portable file format",
                inputPath.GetString().c_str(), texAssetPath.c_str()
        ));
    } else if (_basicUSDZImageFormats.find(texFileExt) == _basicUSDZImageFormats.end()) {
        _failedChecks.emplace_back(TfStringPrintf(
                "Texture <%s> with asset @%s@ has an unknown/unsupported file format.",
                inputPath.GetString().c_str(), texAssetPath.c_str()
        ));
    }
}

void UsdUtilsPrimEncapsulationChecker::ResetCaches() {
    _hasGprimInPathMap.clear();
    _connectableAncestorMap.clear();
}

void UsdUtilsPrimEncapsulationChecker::CheckPrim(const UsdPrim &prim) {
    auto parent = prim.GetParent();

    // Of course, we must allow Boundables under other Boundables, so that
    // schemas like UsdGeom.Pointinstancer can nest their prototypes.  But
    // we disallow a PointInstancer under a Mesh just as we disallow a Mesh
    // under a Mesh, for the same reason: we cannot then independently
    // adjust visibility for the two objects, nor can we reasonably compute
    // the parent Mesh's extent.
    if (prim.IsA<UsdGeomBoundable>()) {
        if (parent && HasGprimAncestor(parent)) {
            _failedChecks.emplace_back(TfStringPrintf(
                    "Gprim <%s> has an ancestor prim that is also a Gprim, which is not allowed.",
                    prim.GetPath().GetString().c_str()
            ));
        }
    }

    auto connectable = UsdShadeConnectableAPI(prim);
    // The GetTypeName() check is to work around a bug in
    // ConnectableAPIBehavior registry.
    if (!(connectable && parent && !prim.GetTypeName().IsEmpty())) {
        return;
    }
    auto pConnectable = UsdShadeConnectableAPI(parent);
    if (pConnectable && !pConnectable.IsContainer() &&
        !parent.GetTypeName().IsEmpty()) {
        // It is a violation of the UsdShade OM which enforces
        // encapsulation of connectable prims under a Container-type
        // connectable prim.
        _failedChecks.emplace_back(TfStringPrintf(
                "Connectable %s <%s> cannot reside under "
                "a non-Container Connectable %s",
                prim.GetTypeName().data(),
                prim.GetPath().GetString().c_str(),
                parent.GetTypeName().data()
        ));
    } else if (!connectable) {
        // It's only OK to have a non-connectable parent if all
        // the rest of your ancestors are also non-connectable.  The
        // error message we give is targeted at the most common
        // infraction, using Scope or other grouping prims inside
        // a Container like a Material
        auto connAnStr = FindConnectableAncestor(parent);
        if (connAnStr) {
            _failedChecks.emplace_back(TfStringPrintf(
                    "Connectabe %s <%s> can only have Connectable "
                    "Container ancestors up to %s ancestor <%s>, but "
                    "parent %s is a %s",
                    prim.GetTypeName().data(),
                    prim.GetPath().GetString().c_str(),
                    connAnStr.GetTypeName().data(),
                    connAnStr.GetPath().GetString().c_str(),
                    parent.GetName().data(),
                    parent.GetTypeName().data()
            ));
        }
    }

}

// NOLINTNEXTLINE
bool UsdUtilsPrimEncapsulationChecker::HasGprimAncestor(const UsdPrim &prim) {
    auto path = prim.GetPath();
    if (_hasGprimInPathMap.find(path) != _hasGprimInPathMap.end()) {
        return _hasGprimInPathMap[path];
    }

    if (path == SdfPath::AbsoluteRootPath()) {
        _hasGprimInPathMap[path] = false;
        return false;
    }

    auto parent = prim.GetParent();
    auto val = HasGprimAncestor(parent);
    if (!val) {
        val = prim.IsA<UsdGeomGprim>();
    }

    _hasGprimInPathMap[path] = val;
    return val;
}

// NOLINTNEXTLINE
UsdPrim UsdUtilsPrimEncapsulationChecker::FindConnectableAncestor(const UsdPrim &prim) {
    auto path = prim.GetPath();
    if (_connectableAncestorMap.find(path) != _connectableAncestorMap.end()) {
        return _connectableAncestorMap[path];
    }

    if (path == SdfPath::AbsoluteRootPath()) {
        _connectableAncestorMap[path] = UsdPrim();
        return {};
    }

    auto parent = prim.GetParent();
    auto val = FindConnectableAncestor(parent);

    // The GetTypeName() check is to work around a bug in
    // ConnectableAPIBehavior registry.
    if (!val && !prim.GetTypeName().IsEmpty()) {
        auto conn = UsdShadeConnectableAPI(prim);
        if (conn) {
            val = prim;
        }
    }

    _connectableAncestorMap[path] = val;
    return val;
}

void UsdUtilsNormalMapTextureChecker::CheckPrim(const UsdPrim &prim) {
    if (!prim.IsA<UsdShadeShader>()) {
        return;
    }

    UsdShadeShader shader{prim};
    if (!shader) {
        _errors.emplace_back(
                TfStringPrintf("Invalid shader prim <%s>",
                               prim.GetPath().GetString().c_str()));
        return;
    }

    TfToken shaderId;
    shader.GetShaderId(&shaderId);
    // We may have failed to fetch an identifier for asset/source-based
    // nodes. We are only interested in UsdPreviewSurface nodes identified via
    // info:id, so it's not an error
    if (shaderId != _usdPreviewSurface) {
        return;
    }

    TfToken normal{"normal"};
    auto normalInput = shader.GetInput(normal);
    if (!normalInput) {
        return;
    }

    auto valueProducingAttrs = UsdShadeUtils::GetValueProducingAttributes(normalInput);
    if (valueProducingAttrs.empty() || valueProducingAttrs[0].GetPrim() == prim) {
        return;
    }

    auto sourcePrim = valueProducingAttrs[0].GetPrim();

    UsdShadeShader sourceShader{sourcePrim};
    if (!sourceShader) {
        // In theory, could be connected to an interface attribute of a
        // parent connectable... not useful, but not an error
        if (UsdShadeConnectableAPI(sourcePrim)) {
            return;
        }
        _failedChecks.emplace_back(TfStringPrintf(
                "%s on prim <%s> is connected to a non-Shader prim.",
                _usdPreviewSurface.data(),
                normal.data()
        ));
        return;
    }

    TfToken sourceId;
    sourceShader.GetShaderId(&sourceId);

    // We may have failed to fetch an identifier for asset/source-based
    // nodes. OR, we could potentially be driven by a UsdPrimvarReader,
    // in which case we'd have nothing to validate
    if (sourceId.IsEmpty() || sourceId != _usdUVTexture) {
        return;
    }

    auto texAssetInput = sourceShader.GetInput(TfToken("file"));
    SdfAssetPath texAsset;
    if (!texAssetInput || !texAssetInput.Get(&texAsset, UsdTimeCode::EarliestTime()) ||
        texAsset.GetResolvedPath().empty()) {
        _failedChecks.emplace_back(TfStringPrintf(
                "%s prim <%s> has invalid or unresolvable inputs:file of @%s@",
                _usdUVTexture.data(),
                sourcePrim.GetPath().GetString().c_str(),
                texAsset.GetResolvedPath().c_str()
        ));
        return;
    }

    if (!TextureIs8Bit(texAsset)) {
        // really nothing more is required for image depths > 8 bits,
        // which we assume FOR NOW, are floating point
        return;
    }

    // -- 8-bit texture validations --
    auto colorSpaceInput = sourceShader.GetInput(TfToken("sourceColorSpace"));
    TfToken colorSpace;
    if (!colorSpaceInput || !colorSpaceInput.Get(&colorSpace, UsdTimeCode::EarliestTime()) ||
        colorSpace != TfToken("raw")) {
        _errors.emplace_back(
                TfStringPrintf("%s prim <%s> that reads Normal Map @%s@ should "
                               "set inputs:sourceColorSpace to 'raw'.",
                               _usdUVTexture.data(),
                               sourcePrim.GetPath().GetString().c_str(),
                               texAsset.GetResolvedPath().c_str())
        );
    }

    GfVec4f bias{};
    auto biasInput = sourceShader.GetInput(TfToken("bias"));
    GfVec4f scale{};
    auto scaleInput = sourceShader.GetInput(TfToken("scale"));

    if (!biasInput || !biasInput.Get(&bias, UsdTimeCode::EarliestTime()) ||
        !scaleInput || !scaleInput.Get(&scale, UsdTimeCode::EarliestTime())) {
        _errors.emplace_back(
                TfStringPrintf("%s prim <%s> reads 8 bit Normal Map @%s@, "
                               "which requires that inputs:scale be set to "
                               "(2, 2, 2, 1) and inputs:bias be set to "
                               "(-1, -1, -1, 0) for proper interpretation as per "
                               "the UsdPreviewSurface and UsdUVTexture docs.",
                               _usdUVTexture.data(),
                               sourcePrim.GetPath().GetString().c_str(),
                               texAsset.GetResolvedPath().c_str()
                )
        );
        return;
    }

    // We still warn for inputs:scale not conforming to UsdPreviewSurface
    // guidelines, as some authoring tools may rely on this to scale an
    // effect of normal perturbations don't really care about fourth components...
    auto nonCompliantScaleValues = (scale[0] != 2 || scale[1] != 2 || scale[2] != 2);
    if (nonCompliantScaleValues) {
        _warnings.emplace_back(TfStringPrintf("%s prim <%s> reads an 8 bit Normal Map, "
                                              "but has non-standard inputs:scale value of %f %f %f."
                                              "inputs:scale must be set to (2, 2, 2, 1) so as "
                                              "fullfill the requirements of the normals to be "
                                              "in tangent space of [(-1,-1,-1), (1,1,1)] as "
                                              "documented in the UsdPreviewSurface and "
                                              "UsdUVTexture docs.",
                                              _usdUVTexture.data(),
                                              sourcePrim.GetPath().GetString().c_str(),
                                              scale[0], scale[1], scale[2]
        ));
    }

    // Note that for a 8bit normal map, inputs:bias must be appropriately
    // set to [-1, -1, -1, 0] to fullfill the requirements of the
    // normals to be in tangent space of [(-1,-1,-1), (1,1,1)] as documented
    // in the UsdPreviewSurface docs. Note this is true only when scale
    // values are respecting the requirements laid in the
    // UsdPreviewSurface / UsdUVTexture docs. We continue to warn!
    if (!nonCompliantScaleValues &&
        (bias[0] != -1 || bias[1] != -1 || bias[2] != -1)) {
        _errors.emplace_back(TfStringPrintf(
                "%s prim <%s> reads an 8 bit Normal Map, but has "
                "non-standard inputs:bias value of %f %f %f. inputs:bias "
                "must be set to [-1,-1,-1,0] so as to fullfill "
                "the requirements of the normals to be in tangent "
                "space of [(-1,-1,-1), (1,1,1)] as documented "
                "in the UsdPreviewSurface and UsdUVTexture docs.",
                _usdUVTexture.data(),
                sourcePrim.GetPath().GetString().c_str(),
                bias[0], bias[1], bias[2]
        ));
    }
}

bool UsdUtilsNormalMapTextureChecker::TextureIs8Bit(SdfAssetPath &asset) {
    auto ext = ArGetResolver().GetExtension(asset.GetResolvedPath());
    return _8bitExtensions.find(ext) != _8bitExtensions.end();
}


void UsdUtilsMaterialBindingAPIAppliedChecker::CheckPrim(const UsdPrim &prim) {
    int numMaterialBindings = 0;
    for (const auto &rel: prim.GetRelationships()) {
        if (rel.GetName() == UsdShadeTokens->materialBinding) {
            ++numMaterialBindings;
        }
    }

    if (numMaterialBindings && !prim.HasAPI<UsdShadeMaterialBindingAPI>()) {
        _failedChecks.emplace_back(TfStringPrintf("Found material bindings but no "
                                                  "MaterialBindingAPI applied on the prim <%s>.",
                                                  prim.GetPath().GetString().c_str()));
    }

}

void UsdUtilsSkelBindingAPIAppliedChecker::CheckPrim(const UsdPrim &prim) {
    if (_skelBindingAPIProps.empty()) {
        auto &usdSchemaRegistry = UsdSchemaRegistry::GetInstance();
        TfTokenVector apis{TfToken("SkelBindingAPI")};
        auto primDef = usdSchemaRegistry.BuildComposedPrimDefinition(TfToken(), apis);
        _skelBindingAPIProps = primDef->GetPropertyNames();
    }

    if (!prim.HasAPI<UsdSkelBindingAPI>()) {
        auto primProperties = prim.GetPropertyNames();
        for (const auto &skelProperty: _skelBindingAPIProps) {
            if (std::find(primProperties.begin(), primProperties.end(), skelProperty) != primProperties.end()) {
                _failedChecks.emplace_back(TfStringPrintf(
                        "Found a UsdSkelBinding property (%s), but no SkelBindingAPI "
                        "applied on the prim <%s>", skelProperty.data(), prim.GetPath().GetString().c_str()
                ));
                return;
            }
        }
        return;
    }

    // If the API is already applied make sure this prim is either
    // SkelRoot type or is rooted under a SkelRoot prim, else prim won't
    // be considered for any UsdSkel Skinning.
    if (prim.GetTypeName() == UsdSkelTokens->SkelRoot) {
        return;
    }

    auto parentPrim = prim.GetParent();
    while (!parentPrim.IsPseudoRoot()) {
        if (parentPrim.GetTypeName() == UsdSkelTokens->SkelRoot) {
            return;
        }

        parentPrim = parentPrim.GetParent();
    }

    _failedChecks.emplace_back(TfStringPrintf(
            "UsdSkelBindingAPI applied on a prim <%s>, which "
            "is not of type SkelRoot or is not rooted at a prim of "
            "type SkelRoot, as required by the UsdSkel schema.",
            prim.GetPath().GetString().c_str()
    ));

}

void UsdUtilsARKitPackageEncapsulationChecker::CheckDependencies(const UsdStageRefPtr &stage,
                                                                 const std::vector<SdfLayerRefPtr> &allLayers,
                                                                 const std::vector<std::string> &allAssets) {
    auto rootLayer = stage->GetRootLayer();
    if (!(rootLayer->GetFileFormat()->IsPackage() || ArIsPackageRelativePath(rootLayer->GetIdentifier()))) {
        return;
    }

    auto packagePath = stage->GetRootLayer()->GetRealPath();
    if (packagePath.empty()) {
        return;
    }

    if (ArIsPackageRelativePath(packagePath)) {
        packagePath = ArSplitPackageRelativePathOuter(packagePath).first;
    }

    for (const auto &layer: allLayers) {
        // In-memory layers like session layers (which we must skip when
        // doing this check) won't have a real path.
        auto realPath = layer->GetRealPath();
        if (!realPath.empty() && !TfStringStartsWith(realPath, packagePath)) {
            _failedChecks.emplace_back(TfStringPrintf(
                    "Found loaded layer '%s' that "
                    "does not belong to the package '%s'.",
                    layer->GetIdentifier().c_str(),
                    packagePath.c_str()
            ));
        }
    }

    for (const auto &asset: allAssets) {
        if (!TfStringStartsWith(asset, packagePath)) {
            _failedChecks.emplace_back(TfStringPrintf(
                    "Found asset reference '%s' that "
                    "does not belong to the package '%s'.",
                    asset.c_str(), packagePath.c_str()
            ));
        }
    }
}

void UsdUtilsARKitLayerChecker::CheckLayer(const SdfLayerRefPtr &layer) {
    Msg(TfStringPrintf("Checking layer <%s>.", layer->GetIdentifier().c_str()));

    auto formatId = layer->GetFileFormat()->GetFormatId().GetString();
    if (_allowedLayerFormatIds.find(formatId) == _allowedLayerFormatIds.end()) {
        _failedChecks.emplace_back(TfStringPrintf(
                "Layer '%s' has unsupported formatId '%s'.",
                layer->GetIdentifier().c_str(), formatId.c_str()
        ));
    }
}

void UsdUtilsARKitPrimTypeChecker::CheckPrim(const UsdPrim &prim) {
    Msg(TfStringPrintf("Checking prim <%s>.", prim.GetPath().GetString().c_str()));
    auto primType = prim.GetTypeName().GetString();
    if (_allowedPrimTypeNames.find(primType) == _allowedPrimTypeNames.end()) {
        _failedChecks.emplace_back(TfStringPrintf("Prim <%s> has unsupported type '%s'.",
                                                  prim.GetPath().GetString().c_str(),
                                                  primType.c_str()));
    }
}

void UsdUtilsARKitShaderChecker::CheckPrim(const UsdPrim &prim) {
    if (!prim.IsA<UsdShadeShader>()) {
        return;
    }

    UsdShadeShader shader{prim};
    if (!shader) {
        // Error has already been issued by a Base-level checker
        return;
    }

    Msg(TfStringPrintf("Checking shader <%s>.", prim.GetPath().GetString().c_str()));

    auto implSource = shader.GetImplementationSource();
    if (implSource != UsdShadeTokens->id) {
        _failedChecks.emplace_back(TfStringPrintf(
                "Shader <%s> has non-id implementation source '%s'.",
                prim.GetPath().GetString().c_str(), implSource.data()
        ));
    }

    TfToken shaderId;
    shader.GetShaderId(&shaderId);

    if (shaderId.IsEmpty() || !(
            _allowedShaderIds.find(shaderId.GetString()) != _allowedShaderIds.end() ||
            TfStringStartsWith(shaderId.GetString(), "UsdPrimvarReader")
    )) {
        _failedChecks.emplace_back(TfStringPrintf("Shader <%s> has unsupported info:id '%s'.",
                                                  prim.GetPath().GetString().c_str(), shaderId.data()));
    }

    // Check shader input connections
    auto shaderInputs = shader.GetInputs();
    for (const auto &shdInput: shaderInputs) {
        SdfPathVector connections;
        shdInput.GetAttr().GetConnections(&connections);

        // If an input has one or more connections, ensure that the
        // connections are valid.
        if (connections.empty()) {
            continue;
        }

        if (connections.size() > 1) {
            _failedChecks.emplace_back(TfStringPrintf(
                    "Shader input <%s> has %zu connection "
                    "sources, but only one is allowed.",
                    shdInput.GetAttr().GetPath().GetString().c_str(),
                    connections.size()
            ));
        }

        UsdShadeConnectableAPI source;
        TfToken sourceName;
        UsdShadeAttributeType sourceType;
        if (!shdInput.GetConnectedSource(&source, &sourceName, &sourceType)) {
            _failedChecks.emplace_back(TfStringPrintf(
                    "Connection source <%s> for shader input <%s> is missing.",
                    connections[0].GetString().c_str(),
                    shdInput.GetAttr().GetPath().GetString().c_str()
            ));
            continue;
        }

        // The source must be a valid shader or material prim.
        auto sourcePrim = source.GetPrim();
        if (!sourcePrim.IsA<UsdShadeShader>() && !sourcePrim.IsA<UsdShadeMaterial>()) {
            _failedChecks.emplace_back(TfStringPrintf(
                    "Shader input <%s> has an invalid "
                    "connection source prim of type '%s'.",
                    shdInput.GetAttr().GetPath().GetString().c_str(),
                    sourcePrim.GetTypeName().data()
            ));
        }


    }
}

void UsdUtilsARKitMaterialBindingChecker::CheckPrim(const UsdPrim &prim) {

    auto relationships = prim.GetRelationships();
    for (const auto &rel: relationships) {
        if (!TfStringStartsWith(rel.GetName().GetString(), UsdShadeTokens->materialBinding.GetString())) {
            continue;
        }
        SdfPathVector targets;
        rel.GetTargets(&targets);

        if (targets.size() == 1) {
            auto directBinding = UsdShadeMaterialBindingAPI::DirectBinding(rel);
            if (!directBinding.GetMaterial()) {
                _failedChecks.emplace_back(TfStringPrintf(
                        "Direct material binding <%s> targets "
                        "an invalid material <%s>.",
                        rel.GetPath().GetString().c_str(),
                        directBinding.GetMaterialPath().GetString().c_str()
                ));
            }
        } else if (targets.size() == 2) {
            auto collBinding = UsdShadeMaterialBindingAPI::CollectionBinding(rel);
            if (!collBinding.GetMaterial()) {
                _failedChecks.emplace_back(TfStringPrintf(
                        "Collection-based material binding "
                        "<%s> targets an invalid material <%s>.",
                        rel.GetPath().GetString().c_str(),
                        collBinding.GetMaterialPath().GetString().c_str()
                ));
            }

            if (!collBinding.GetCollection()) {
                _failedChecks.emplace_back(TfStringPrintf(
                        "Collection-based material binding "
                        "<%s> targets an invalid collection <%s>.",
                        rel.GetPath().GetString().c_str(),
                        collBinding.GetMaterialPath().GetString().c_str()
                ));
            }

        }
    }
}

void UsdUtilsARKitFileExtensionChecker::CheckZipFile(const UsdZipFile &zipFile, const std::string &packagePath) {
    for (auto file = zipFile.begin(); file != zipFile.end(); ++file) {
        auto fileExt = ArGetResolver().GetExtension(*file);
        if (_allowedExtensions.find(fileExt) == _allowedExtensions.end()) {
            _failedChecks.emplace_back(TfStringPrintf(
                                               "File '%s' in package '%s' has an "
                                               "unknown or unsupported extension '%s'.",
                                               file->c_str(), packagePath.c_str(), fileExt.c_str()
                                       )
            );
        }
    }

}

void UsdUtilsARKitRootLayerChecker::CheckStage(const UsdStageRefPtr &stage) {
    auto usedLayers = stage->GetUsedLayers();
    std::vector<SdfLayerRefPtr> usedLayersOnDisk;
    // This list exclused any session layers
    for (const auto &layer: usedLayers) {
        if (!layer->GetRealPath().empty()) {
            usedLayersOnDisk.emplace_back(layer);
        }
    }

    if (usedLayersOnDisk.size() > 1) {
        _failedChecks.emplace_back(TfStringPrintf(
                "The stage uses %zu layers. It should "
                "contain a single usdc layer to be compatible with ARKit's "
                "implementation of usdz.",
                usedLayersOnDisk.size()
        ));
    }

    auto rootLayerRealPath = stage->GetRootLayer()->GetRealPath();
    if (TfStringEndsWith(rootLayerRealPath, ".usdz")) {
        auto zipFile = UsdZipFile::Open(rootLayerRealPath);
        if (!zipFile) {
            _errors.emplace_back(TfStringPrintf("Coudl not open package at path '%s'", rootLayerRealPath.c_str()));
            return;
        }
        if (!TfStringEndsWith(*zipFile.begin(), ".usdc")) {
            _failedChecks.emplace_back(TfStringPrintf("First file (%s) in usdz package '%s' "
                                                      "does not have the .usdc extension.",
                                                      zipFile.begin()->c_str(),
                                                      rootLayerRealPath.c_str()));
        }
    } else if (!TfStringEndsWith(rootLayerRealPath, ".usdc")) {
        _failedChecks.emplace_back(TfStringPrintf("Root layer of the stage '%s' does not "
                                                  "have the '.usdc' extension.", rootLayerRealPath.c_str()));
    }
}

UsdUtilsComplianceChecker::UsdUtilsComplianceChecker(bool arkit,
                                                     bool skipARKitRootLayerCheck,
                                                     bool rootPackageOnly,
                                                     bool skipVariants,
                                                     bool verbose,
                                                     bool assetLevelChecks) :
        _arkit(arkit),
        _rootPackageOnly(rootPackageOnly),
        _skipVariants(skipVariants),
        _verbose(verbose),
        _assetLevelChecks(assetLevelChecks) {
    _rules = {
            new UsdUtilsByteAlignmentChecker(_verbose, _arkit, _assetLevelChecks),
            new UsdUtilsCompressionChecker(_verbose, _arkit, _assetLevelChecks),
            new UsdUtilsMissingReferenceChecker(_verbose, _arkit, _assetLevelChecks),
            new UsdUtilsStageMetadataChecker(_verbose, _arkit, _assetLevelChecks),
            new UsdUtilsTextureChecker(_verbose, _arkit, _assetLevelChecks),
            new UsdUtilsPrimEncapsulationChecker(_verbose, _arkit, _assetLevelChecks),
            new UsdUtilsNormalMapTextureChecker(_verbose, _arkit, _assetLevelChecks),
            new UsdUtilsMaterialBindingAPIAppliedChecker(_verbose, _arkit, _assetLevelChecks),
            new UsdUtilsSkelBindingAPIAppliedChecker(_verbose, _arkit, _assetLevelChecks)
    };

    if (arkit) {
        _rules.insert(_rules.end(),
                      {
                              new UsdUtilsARKitLayerChecker(_verbose, _arkit, _assetLevelChecks),
                              new UsdUtilsARKitPrimTypeChecker(_verbose, _arkit, _assetLevelChecks),
                              new UsdUtilsARKitShaderChecker(_verbose, _arkit, _assetLevelChecks),
                              new UsdUtilsARKitMaterialBindingChecker(_verbose, _arkit, _assetLevelChecks),
                              new UsdUtilsARKitFileExtensionChecker(_verbose, _arkit, _assetLevelChecks),
                              new UsdUtilsARKitPackageEncapsulationChecker(_verbose, _arkit, _assetLevelChecks)
                      });
        if (!skipARKitRootLayerCheck) {
            _rules.emplace_back(new UsdUtilsARKitRootLayerChecker(_verbose, _arkit, _assetLevelChecks));
        }
    }


}

UsdUtilsComplianceChecker::~UsdUtilsComplianceChecker() {
    for (auto rule: _rules) {
        delete rule;
    }
}

void UsdUtilsComplianceChecker::Msg(const std::string &msg) const {
    if (_verbose) {
        std::cout << msg << std::endl;
    }
}

void UsdUtilsComplianceChecker::DumpRules() {
    std::cout << "Checking rules: " << std::endl;
    for (auto rule: _rules) {
        std::cout << std::string(10, '-') << std::endl;
        std::cout << "[" << rule->GetName() << "]:\n "
                  << rule->GetDescription() << std::endl;

    }
    std::cout << std::string(10, '-') << std::endl;
}

std::vector<std::string> UsdUtilsComplianceChecker::GetWarnings() {
    std::vector<std::string> warnings;
    warnings.reserve(_warnings.size());
    for (const auto &warning: _warnings) {
        warnings.emplace_back(warning);
    }
    for (auto rule: _rules) {
        auto name = rule->GetName();
        for (const auto &warning: rule->GetWarnings()) {
            warnings.emplace_back(
                    TfStringPrintf("%s (may violate '%s')",
                                   warning.c_str(), name.c_str())
            );
        }
    }
    return warnings;
}

std::vector<std::string> UsdUtilsComplianceChecker::GetErrors() {
    std::vector<std::string> errors;
    errors.reserve(_errors.size());
    for (const auto &error: _errors) {
        errors.emplace_back(error);
    }
    for (auto rule: _rules) {
        auto name = rule->GetName();
        for (const auto &error: rule->GetErrors()) {
            errors.emplace_back(TfStringPrintf("Error checking rule '%s': %s", name.c_str(), error.c_str()));
        }
    }
    return errors;
}

std::vector<std::string> UsdUtilsComplianceChecker::GetFailedChecks() {
    std::vector<std::string> failedChecks;
    failedChecks.reserve(_failedChecks.size());
    for (const auto &failedCheck: _failedChecks) {
        failedChecks.emplace_back(failedCheck);
    }
    for (auto rule: _rules) {
        auto name = rule->GetName();
        for (const auto &failedCheck: rule->GetFailedChecks()) {
            failedChecks.emplace_back(TfStringPrintf("%s (fails '%s')", failedCheck.c_str(), name.c_str()));
        }
    }
    return failedChecks;

}

void UsdUtilsComplianceChecker::CheckCompliance(const std::string &inputFile) {
    for (auto rule: _rules) {
        rule->ResetCaches();
    }

    if (!UsdStage::IsSupportedFile(inputFile)) {
        _errors.emplace_back(TfStringPrintf("Cannot open file '%s' on a USD stage.", inputFile.c_str()));
        return;
    }

    auto delegate = UsdUtilsCoalescingDiagnosticDelegate();
    if (_verbose) {
        std::cout << "Opening " << inputFile << std::endl;
    }

    auto stage = UsdStage::Open(inputFile);
    auto stageOpenDiagnostics = delegate.TakeUncoalescedDiagnostics();

    for (auto rule: _rules) {
        rule->CheckStage(stage);
        rule->CheckDiagnostics(stageOpenDiagnostics);
    }

    // Create resolver context
    ArResolver &resolver = ArGetResolver();
    auto context = resolver.CreateDefaultContext();
    auto binder = ArResolverContextBinder(context);

    // Recursively compute all external dependencies
    std::vector<SdfLayerRefPtr> allLayers;
    std::vector<std::string> allAssets;
    std::vector<std::string> unresolvedPaths;
    if (!UsdUtilsComputeAllDependencies(SdfAssetPath(inputFile), &allLayers, &allAssets, &unresolvedPaths)) {
        _errors.emplace_back(TfStringPrintf("Failed to get dependencies of %s.", inputFile.c_str()));
        return;
    }

    for (auto rule: _rules) {
        rule->CheckUnresolvedPaths(unresolvedPaths);
        rule->CheckDependencies(stage, allLayers, allAssets);
    }

    if (_rootPackageOnly) {
        auto rootLayer = stage->GetRootLayer();
        if (rootLayer->GetFileFormat()->IsPackage()) {
            auto packagePath = ArSplitPackageRelativePathInner(rootLayer->GetIdentifier()).first;
            CheckPackage(packagePath);
        } else {
            _errors.emplace_back(TfStringPrintf("Root layer of the USD stage (%s) doesn't belong to "
                                                "a package, but 'rootPackageOnly' is True!",
                                                UsdDescribe(stage).c_str()));
        }
    } else {
        std::set<std::string> packages;
        for (const auto &layer: allLayers) {
            if (layer->GetFileFormat()->IsPackage() || ArIsPackageRelativePath(layer->GetIdentifier())) {
                auto packagePath = ArSplitPackageRelativePathInner(layer->GetIdentifier()).first;
                packages.insert(packagePath);
            }
            CheckLayer(layer);
        }

        for (const auto &package: packages) {
            CheckPackage(package);
        }

        stage->SetEditTarget(stage->GetSessionLayer());
        auto primRange = UsdPrimRange::Stage(stage, UsdTraverseInstanceProxies());

        TraverseRange(primRange, true);
    }
}

// NOLINTNEXTLINE
void UsdUtilsComplianceChecker::CheckPackage(const std::string &packagePath) {
    Msg(TfStringPrintf("Checkign package <%s>", packagePath.c_str()));

    // XXX: Should we open the package on a stage to ensure that it is valid
    // and entirely self-contained

    auto pkgExt = ArGetResolver().GetExtension(packagePath);
    if (pkgExt != "usdz") {
        _errors.emplace_back(
                TfStringPrintf("Package at path %s has an invalid extension",
                               packagePath.c_str()));
        return;
    }

    // Check the parent package first
    if (ArIsPackageRelativePath(packagePath)) {
        auto parentPackagePath = ArSplitPackageRelativePathInner(packagePath).first;
        CheckPackage(parentPackagePath);
    }

    // Avoid checking the same parent package multiple times
    if (_checkedPackages.find(packagePath) != _checkedPackages.end()) {
        return;
    }

    _checkedPackages.insert(packagePath);

    auto resolvedPath = ArGetResolver().Resolve(packagePath);
    if (!resolvedPath) {
        _errors.emplace_back(
                TfStringPrintf("Failed to resolve package path '%s'", packagePath.c_str())
        );
        return;
    }

    auto zipfile = UsdZipFile::Open(packagePath);
    if (!zipfile) {
        _errors.emplace_back(
                TfStringPrintf("Could not open package at path '%s'", resolvedPath.GetPathString().c_str())
        );
        return;
    }

    for (auto rule: _rules) {
        rule->CheckZipFile(zipfile, packagePath);
    }

}

void UsdUtilsComplianceChecker::CheckLayer(const SdfLayerRefPtr &layer) {
    for (auto rule: _rules) {
        rule->CheckLayer(layer);
    }
}

void UsdUtilsComplianceChecker::CheckPrim(const UsdPrim &prim) {
    for (auto rule: _rules) {
        rule->CheckPrim(prim);
    }
}

// NOLINTNEXTLINE
void UsdUtilsComplianceChecker::TraverseRange(UsdPrimRange &primRange, bool isStageRoot) {
    std::vector<UsdPrim> primsWithVariants;
    auto rootPrim = primRange.begin()->GetPrim();
    for (auto iter = primRange.begin(); iter != primRange.end(); ++iter) {
        auto prim = iter->GetPrim();
        if (_skipVariants || (!isStageRoot && prim == rootPrim)) {
            CheckPrim(prim);
            continue;
        }

        auto vSets = prim.GetVariantSets();
        auto vSetNames = vSets.GetNames();
        if (vSetNames.empty()) {
            CheckPrim(prim);
        } else {
            primsWithVariants.emplace_back(prim);
            iter.PruneChildren();
        }
    }

    for (auto prim: primsWithVariants) {
        TraverseVariants(prim);
    }

}

// NOLINTNEXTLINE
void UsdUtilsComplianceChecker::TraverseVariants(UsdPrim &prim) {
    if (prim.IsInstanceProxy()) {
        return;
    }

    auto vSets = prim.GetVariantSets();
    auto vSetNames = vSets.GetNames();
    std::vector<std::vector<std::string>> allVariantNames;

    for (const auto &vSetName: vSetNames) {
        auto vSet = vSets.GetVariantSet(vSetName);
        auto vNames = vSet.GetVariantNames();
        allVariantNames.emplace_back(vNames);
    }

    auto allVariations = CartesianProduct(allVariantNames);
    for (auto variation: allVariations) {
        for (uint idx = 0; idx < variation.size(); ++idx) {
            vSets.SetSelection(vSetNames[idx], variation[idx]);
        }

        for (auto rule: _rules) {
            rule->ResetCaches();
        }

        auto primRange = UsdPrimRange(prim, UsdTraverseInstanceProxies());
        TraverseRange(primRange);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE