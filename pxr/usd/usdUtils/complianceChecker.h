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

#ifndef PXR_USD_USD_UTILS_COMPLIANCECHECKER_H
#define PXR_USD_USD_UTILS_COMPLIANCECHECKER_H

#include <string>
#include <vector>
#include <set>
#include <map>

#include "pxr/base/tf/diagnosticBase.h"
#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/zipFile.h"


PXR_NAMESPACE_OPEN_SCOPE

class UsdUtilsBaseRuleChecker {
public:
    UsdUtilsBaseRuleChecker(bool verbose,
                            bool consumerLevelChecks,
                            bool assetLevelChecks
    );

    virtual ~UsdUtilsBaseRuleChecker() = default;


    std::vector<std::string> GetFailedChecks() { return _failedChecks; }

    std::vector<std::string> GetErrors() { return _errors; }

    std::vector<std::string> GetWarnings() { return _warnings; }

public:
    // Virtual methods that must be overriden
    virtual std::string GetName() = 0;

    virtual std::string GetDescription() = 0;

    // Optional Virtual Methods
    virtual void ResetCaches() {};

    virtual void CheckStage(const UsdStageRefPtr &stage) {}

    virtual void CheckPrim(const UsdPrim &prim) {}

    virtual void CheckDiagnostics(const std::vector<std::unique_ptr<TfDiagnosticBase>> &diagnostics) {}

    virtual void CheckUnresolvedPaths(const std::vector<std::string> &unresolvedPaths) {}

    virtual void CheckDependencies(const UsdStageRefPtr &stage,
                                   const std::vector<SdfLayerRefPtr> &allLayers,
                                   const std::vector<std::string> &allAssets) {}

    virtual void CheckZipFile(const UsdZipFile &zipFile, const std::string &packagePath) {}

    virtual void CheckLayer(const SdfLayerRefPtr &layer) {};
protected:
    void Msg(const std::string &msg) const;

    bool _verbose;
    bool _consumerLevelChecks;
    bool _assetLevelChecks;
    std::vector<std::string> _failedChecks;
    std::vector<std::string> _errors;
    std::vector<std::string> _warnings;
};

class UsdUtilsByteAlignmentChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "ByteAlignmentChecker"; }

    std::string GetDescription() final {
        return ("Files within a usdz package must be laid out properly, "
                "i.e. they should be aligned to 64 bytes.");
    }

    void CheckZipFile(const UsdZipFile &zipFile, const std::string &packagePath) final;
};

class UsdUtilsCompressionChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "CompressionChecker"; }

    std::string GetDescription() final {
        return ("Files within a usdz package should not be compressed or encrypted.");
    }

    void CheckZipFile(const UsdZipFile &zipFile, const std::string &packagePath) final;
};

class UsdUtilsMissingReferenceChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "MissingReferenceChecker"; }

    std::string GetDescription() final {
        return ("The composed USD stage should not contain any unresolvable"
                " asset dependencies (in every possible variation of the "
                "asset), when using the default asset resolver. ");
    }

    void CheckDiagnostics(const std::vector<std::unique_ptr<TfDiagnosticBase>> &diagnostics) final;

    void CheckUnresolvedPaths(const std::vector<std::string> &unresolvedPaths) final;

};

class UsdUtilsStageMetadataChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "StageMetadataChecker"; }

    std::string GetDescription() final {
        return ("All stages should declare their 'upAxis' and 'metersPerUnit'. "
                "Stages that can be consumed as referencable assets should furthermore have"
                "a valid 'defaultPrim' declared, and stages meant for consumer-level packaging"
                "should always have upAxis set to 'Y' ");
    }

    void CheckStage(const UsdStageRefPtr &stage) final;


};

class UsdUtilsTextureChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "TextureChecker"; }

    std::string GetDescription() final {
        return ("Texture files should be readable by intended client "
                "(only .jpg, .jpeg or .png for consumer-level USDZ).");
    }

    void CheckStage(const UsdStageRefPtr &stage) final;

    void CheckPrim(const UsdPrim &prim) final;

private:
    void CheckTexture(const std::string &texAssetPath, const SdfPath &inputPath);

private:
    bool _checkBaseUSDZFiles = false;
    const std::set<std::string> _basicUSDZImageFormats{"exr", "jpg", "jpeg", "png"};
    const std::set<std::string> _unsupportedImageFormats{"bmp", "tga", "hdr", "tif", "tx", "zfile"};
};

class UsdUtilsPrimEncapsulationChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "PrimEncapsulationChecker"; }

    std::string GetDescription() final {
        return ("Check for basic prim encapsulation rules:"
                "- Boundables may not be nested under Gprims"
                "- Connectable prims (e.g. Shader, Material, etc) can only be nested"
                "inside other Container-like Connectable prims. Container-like prims"
                "include Material, NodeGraph, Light, LightFilter, and *exclude Shader*");
    }

    void CheckPrim(const UsdPrim &prim) final;

    void ResetCaches() final;

private:
    bool HasGprimAncestor(const UsdPrim &prim);

    UsdPrim FindConnectableAncestor(const UsdPrim &prim);

    std::map<SdfPath, bool> _hasGprimInPathMap;
    std::map<SdfPath, UsdPrim> _connectableAncestorMap;
};

class UsdUtilsNormalMapTextureChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "NormalMapTextureChecker"; }

    std::string GetDescription() final {
        return ("UsdUVTexture nodes that feed the _inputs:normals_ of a"
                "UsdPreviewSurface must ensure that the data is encoded and scaled properly."
                "Specifically:"
                "- Since normals are expected to be in the range [(-1,-1,-1), (1,1,1)],"
                "the Texture node must transform 8-bit textures from their [0..1] range by"
                "setting its _inputs:scale_ to (2, 2, 2, 1) and"
                "_inputs:bias_ to (-1, -1, -1, 0)"
                "- Normal map data is commonly expected to be linearly encoded.  However, many"
                "image-writing tools automatically set the profile of three-channel, 8-bit"
                "images to SRGB.  To prevent an unwanted transformation, the UsdUVTexture's"
                " _inputs:sourceColorSpace_ must be set to 'raw'");
    }

    void CheckPrim(const UsdPrim &prim) final;

private:
    bool TextureIs8Bit(SdfAssetPath &asset);

    const std::set<std::string> _8bitExtensions{"bmp", "tga", "jpg", "jpeg", "png", "tif"};
    const TfToken _usdPreviewSurface{"UsdPreviewSurface"};
    const TfToken _usdUVTexture{"UsdUVTexture"};
};

class UsdUtilsMaterialBindingAPIAppliedChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "MaterialBindingAPIAppliedChecker"; }

    std::string GetDescription() final {
        return ("A prim providing a material binding, must have "
                "MaterialBindingAPI applied on the prim.");
    }

    void CheckPrim(const UsdPrim &prim) final;
};

class UsdUtilsSkelBindingAPIAppliedChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "SkelBindingAPIAppliedChecker"; }

    std::string GetDescription() final {
        return ("A prim providing skelBinding properties, must have "
                "SkelBindingAPI applied on the prim.");
    }

    void CheckPrim(const UsdPrim &prim) final;

private:
    std::vector<TfToken> _skelBindingAPIProps;
};

class UsdUtilsARKitPackageEncapsulationChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "ARKitPackageEncapsulationChecker"; }

    std::string GetDescription() final {
        return ("If the root layer is a package, then the composed stage "
                "should not contain references to files outside the package. "
                "In other words, the package should be entirely self-contained.");
    }

    void CheckDependencies(const UsdStageRefPtr &stage,
                           const std::vector<SdfLayerRefPtr> &allLayers,
                           const std::vector<std::string> &allAssets) final;
};

class UsdUtilsARKitLayerChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "ARKitLayerChecker"; }

    std::string GetDescription() final {
        return ("All included layers that participate in composition should"
                " have one of the core supported file formats.");
    }

    void CheckLayer(const SdfLayerRefPtr &layer) final;

private:
    std::set<std::string> _allowedLayerFormatIds{"usd", "usda", "usdc", "usdz"};
};

class UsdUtilsARKitPrimTypeChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "ARKitPrimTypeChecker"; }

    std::string GetDescription() final {
        return ("UsdGeomPointInstancers and custom schemas not provided by "
                "core USD are not allowed.");
    }

    void CheckPrim(const UsdPrim &prim) final;

private:
    std::set<std::string> _allowedPrimTypeNames{"", "Scope", "Xform", "Camera",
                                                "Shader", "Material",
                                                "Mesh", "Sphere", "Cube", "Cylinder", "Cone",
                                                "Capsule", "GeomSubset", "Points",
                                                "SkelRoot", "Skeleton", "SkelAnimation",
                                                "BlendShape", "SpatialAudio"};
};

class UsdUtilsARKitShaderChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "ARKitShaderChecker"; }

    std::string GetDescription() final {
        return ("Shader nodes must have \"id\" as the implementationSource, "
                "with id values that begin with \"Usd*\". Also, shader inputs "
                "with connections must each have a single, valid connection "
                "source.");
    }

    void CheckPrim(const UsdPrim &prim) final;

private:
    std::set<std::string> _allowedShaderIds{"UsdPreviewSurface", "UsdUVTexture", "UsdTransform2d"};
};

class UsdUtilsARKitMaterialBindingChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "ARKitMaterialBindingChecker"; }

    std::string GetDescription() final {
        return ("All material binding relationships must have valid targets.");
    }

    void CheckPrim(const UsdPrim &prim) final;
};

class UsdUtilsARKitFileExtensionChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "ARKitFileExtensionChecker"; }

    std::string GetDescription() final {
        return ("Only layer files and textures are allowed in a package.");
    }

    void CheckZipFile(const UsdZipFile &zipFile, const std::string &packagePath) final;

private:
    const std::set<std::string> _allowedExtensions{"exr", "jpg", "jpeg", "png",
                                                   "usd", "usda", "usdc", "usdz"};

};

class UsdUtilsARKitRootLayerChecker : public UsdUtilsBaseRuleChecker {
    using UsdUtilsBaseRuleChecker::UsdUtilsBaseRuleChecker;

public:
    std::string GetName() final { return "ARKitRootLayerChecker"; }

    std::string GetDescription() final {
        return ("The root layer of the package must be a usdc file and "
                "must not include any external dependencies that participate in "
                "stage composition.");
    }

    void CheckStage(const UsdStageRefPtr &stage) final;
};

class UsdUtilsComplianceChecker {
public:
    UsdUtilsComplianceChecker(bool arkit,
                              bool skipARKitRootLayerCheck,
                              bool rootPackageOnly,
                              bool skipVariants,
                              bool verbose,
                              bool assetLevelChecks);

    ~UsdUtilsComplianceChecker();

    void DumpRules();

    void CheckCompliance(const std::string &inputFile);

    std::vector<std::string> GetWarnings();

    std::vector<std::string> GetErrors();

    std::vector<std::string> GetFailedChecks();

private:
    void CheckPackage(const std::string &packagePath);

    void CheckLayer(const SdfLayerRefPtr &layer);

    void CheckPrim(const UsdPrim &prim);

    void TraverseRange(UsdPrimRange &primRange, bool isStageRoot = false);

    void TraverseVariants(UsdPrim &prim);

    void Msg(const std::string &msg) const;

private:
    std::vector<UsdUtilsBaseRuleChecker *> _rules;
    std::vector<std::string> _warnings;
    std::vector<std::string> _errors;
    std::vector<std::string> _failedChecks;
    std::set<std::string> _checkedPackages;
    bool _arkit;
    bool _rootPackageOnly;
    bool _skipVariants;
    bool _verbose;
    bool _assetLevelChecks;
};


PXR_NAMESPACE_CLOSE_SCOPE
#endif //PXR_USD_USD_UTILS_COMPLIANCECHECKER_H
