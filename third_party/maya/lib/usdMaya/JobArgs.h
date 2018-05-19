//
// Copyright 2016 Pixar
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
#ifndef PXRUSDMAYA_JOBARGS_H
#define PXRUSDMAYA_JOBARGS_H

/// \file JobArgs.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"

#include <map>
#include <ostream>
#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


#define PXRUSDMAYA_TRANSLATOR_TOKENS \
    (Collapsed) \
    (Full) \
    (Import) \
    ((UsdFileExtensionDefault, "usd")) \
    ((UsdFileExtensionASCII, "usda")) \
    ((UsdFileExtensionCrate, "usdc")) \
    ((UsdFileFilter, "*.usd *.usda *.usdc"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaTranslatorTokens,
        PXRUSDMAYA_TRANSLATOR_TOKENS);

#define PXRUSDMAYA_JOBARGS_TOKENS \
    (defaultLayer) \
    (currentLayer) \
    (modelingVariant) \
    (none) \
    ((auto_, "auto")) \
    ((explicit_, "explicit"))

TF_DECLARE_PUBLIC_TOKENS(PxUsdExportJobArgsTokens, 
        PXRUSDMAYA_JOBARGS_TOKENS);

struct JobExportArgs
{
    PXRUSDMAYA_API
    JobExportArgs();

    bool exportRefsAsInstanceable;
    bool exportDisplayColor;
    TfToken shadingMode;
    
    bool mergeTransformAndShape;
    bool exportInstances;

    bool exportAnimation;
    bool excludeInvisible;
    bool exportDefaultCameras;
    bool exportSkin;
    bool autoSkelRoots;

    bool exportMeshUVs;
    bool normalizeMeshUVs;
    
    bool exportMaterialCollections;
    std::string materialCollectionsPath;
    bool exportCollectionBasedBindings;

    bool normalizeNurbs;
    bool exportNurbsExplicitUV;
    
    bool exportColorSets;

    TfToken renderLayerMode;
    
    TfToken defaultMeshScheme;

    bool exportVisibility;

    std::string melPerFrameCallback;
    std::string melPostCallback;
    std::string pythonPerFrameCallback;
    std::string pythonPostCallback;

    PxrUsdMayaUtil::ShapeSet dagPaths;

    std::vector<std::string> chaserNames;
    typedef std::map<std::string, std::string> ChaserArgs;
    std::map< std::string, ChaserArgs > allChaserArgs;
    
    // This path is provided when dealing with variants
    // where a _BaseModel_ root path is used instead of
    // the model path. This to allow a proper internal reference
    SdfPath usdModelRootOverridePath;

    TfToken rootKind;

    PXRUSDMAYA_API
    void setParentScope(const std::string& ps);

    const SdfPath& getParentScope() const {
        return parentScope;
    }
private:
    SdfPath parentScope;
};

PXRUSDMAYA_API
std::ostream& operator <<(std::ostream& out, const JobExportArgs& exportArgs);


struct JobImportArgs
{
    PXRUSDMAYA_API
    JobImportArgs();

    TfToken shadingMode;
    TfToken assemblyRep;
    bool readAnimData;
    bool useCustomFrameRange;
    double startTime;
    double endTime;
    bool importWithProxyShapes;
    TfToken::Set includeMetadataKeys;
    TfToken::Set includeAPINames;
};

PXRUSDMAYA_API
std::ostream& operator <<(std::ostream& out, const JobImportArgs& exportArgs);


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYA_JOBARGS_H
