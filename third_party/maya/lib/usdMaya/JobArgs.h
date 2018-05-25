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
    ((UsdFileExtensionDefault, "usd")) \
    ((UsdFileExtensionASCII, "usda")) \
    ((UsdFileExtensionCrate, "usdc")) \
    ((UsdFileFilter, "*.usd *.usda *.usdc"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdMayaTranslatorTokens,
        PXRUSDMAYA_TRANSLATOR_TOKENS);

#define PXRUSDMAYA_JOBEXPORTARGS_TOKENS \
    /* Dictionary keys */ \
    (chaser) \
    (chaserArgs) \
    (defaultCameras) \
    (defaultMeshScheme) \
    (exportCollectionBasedBindings) \
    (exportColorSets) \
    (exportDisplayColor) \
    (exportInstances) \
    (exportMaterialCollections) \
    (exportRefsAsInstanceable) \
    (exportSkin) \
    (exportUVs) \
    (exportVisibility) \
    (kind) \
    (materialCollectionsPath) \
    (melPerFrameCallback) \
    (melPostCallback) \
    (mergeTransformAndShape) \
    (normalizeMeshUVs) \
    (normalizeNurbs) \
    (parentScope) \
    (pythonPerFrameCallback) \
    (pythonPostCallback) \
    (renderableOnly) \
    (renderLayerMode) \
    (shadingMode) \
    /* renderLayerMode values */ \
    (defaultLayer) \
    (currentLayer) \
    (modelingVariant) \
    /* exportSkin values */ \
    (none) \
    ((auto_, "auto")) \
    ((explicit_, "explicit"))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdExportJobArgsTokens, 
        PXRUSDMAYA_JOBEXPORTARGS_TOKENS);

#define PXRUSDMAYA_JOBIMPORTARGS_TOKENS \
    /* Dictionary keys */ \
    (apiSchema) \
    (assemblyRep) \
    (metadata) \
    (shadingMode) \
    /* assemblyRep values */ \
    (Collapsed) \
    (Full) \
    (Import) \
    ((Unloaded, ""))

TF_DECLARE_PUBLIC_TOKENS(PxrUsdImportJobArgsTokens,
        PXRUSDMAYA_JOBIMPORTARGS_TOKENS);

struct JobExportArgs
{
    const TfToken defaultMeshScheme;
    const bool excludeInvisible;
    const bool exportCollectionBasedBindings;
    const bool exportColorSets;
    const bool exportDefaultCameras;
    const bool exportDisplayColor;
    const bool exportInstances;
    const bool exportMaterialCollections;
    const bool exportMeshUVs;
    const bool exportNurbsExplicitUV;
    const bool exportRefsAsInstanceable;
    const TfToken exportSkin;
    const bool exportVisibility;
    const SdfPath materialCollectionsPath;
    const bool mergeTransformAndShape;
    const bool normalizeMeshUVs;
    const bool normalizeNurbs;
    const SdfPath parentScope;
    const TfToken renderLayerMode;
    const TfToken rootKind;
    const TfToken shadingMode;

    typedef std::map<std::string, std::string> ChaserArgs;
    const std::vector<std::string> chaserNames;
    const std::map< std::string, ChaserArgs > allChaserArgs;
    
    const std::string melPerFrameCallback;
    const std::string melPostCallback;
    const std::string pythonPerFrameCallback;
    const std::string pythonPostCallback;

    const PxrUsdMayaUtil::ShapeSet dagPaths;
    /// The interval over which to export animated data.
    /// An empty interval (<tt>GfInterval::IsEmpty()</tt>) means that no
    /// animated (time-sampled) data should be exported.
    /// Otherwise, animated data should be exported at times contained in the
    /// interval.
    const GfInterval timeInterval;

    // This path is provided when dealing with variants
    // where a _BaseModel_ root path is used instead of
    // the model path. This to allow a proper internal reference.
    SdfPath usdModelRootOverridePath; // XXX can we make this const?

    /// Creates a JobExportArgs from the given \p dict, overlaid on top of the
    /// default dictionary given by GetDefaultDictionary().
    /// The values of \p dict are stronger (will override) the values from the
    /// default dictionary.
    /// Issues runtime errors if \p dict contains values of the wrong type;
    /// types should match those declared in GetDefaultDictionary().
    PXRUSDMAYA_API
    static JobExportArgs CreateFromDictionary(
        const VtDictionary& userArgs,
        const PxrUsdMayaUtil::ShapeSet& dagPaths,
        const GfInterval& timeInterval = GfInterval());

    /// Gets the default arguments dictionary for JobExportArgs.
    PXRUSDMAYA_API
    static const VtDictionary& GetDefaultDictionary();

private:
    PXRUSDMAYA_API
    JobExportArgs(
        const VtDictionary& userArgs,
        const PxrUsdMayaUtil::ShapeSet& dagPaths,
        const GfInterval& timeInterval);
};

PXRUSDMAYA_API
std::ostream& operator <<(std::ostream& out, const JobExportArgs& exportArgs);


struct JobImportArgs
{
    const TfToken assemblyRep;
    const TfToken::Set includeAPINames;
    const TfToken::Set includeMetadataKeys;
    TfToken shadingMode; // XXX can we make this const?

    const bool importWithProxyShapes;
    /// The interval over which to import animated data.
    /// An empty interval (<tt>GfInterval::IsEmpty()</tt>) means that no
    /// animated (time-sampled) data should be imported.
    /// A full interval (<tt>timeInterval == GfInterval::GetFullInterval()</tt>)
    /// means to import all available data, though this does not need to be
    /// special-cased because USD will accept full intervals like any other
    /// non-empty interval.
    const GfInterval timeInterval;

    /// Creates a JobImportArgs from the given \p dict, overlaid on top of the
    /// default dictionary given by GetDefaultDictionary().
    /// The values of \p dict are stronger (will override) the values from the
    /// default dictionary.
    /// Issues runtime errors if \p dict contains values of the wrong type;
    /// types should match those declared in GetDefaultDictionary().
    PXRUSDMAYA_API
    static JobImportArgs CreateFromDictionary(
        const VtDictionary& userArgs,
        const bool importWithProxyShapes = false,
        const GfInterval& timeInterval = GfInterval::GetFullInterval());

    /// Gets the default arguments dictionary for JobImportArgs.
    PXRUSDMAYA_API
    static const VtDictionary& GetDefaultDictionary();

private:
    PXRUSDMAYA_API
    JobImportArgs(
        const VtDictionary& userArgs,
        const bool importWithProxyShapes,
        const GfInterval& timeInterval);
};

PXRUSDMAYA_API
std::ostream& operator <<(std::ostream& out, const JobImportArgs& exportArgs);


PXR_NAMESPACE_CLOSE_SCOPE


#endif // PXRUSDMAYA_JOBARGS_H
