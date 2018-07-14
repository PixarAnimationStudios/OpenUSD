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
#include "pxr/pxr.h"
#include "usdMaya/jobArgs.h"

#include "usdMaya/registryHelper.h"
#include "usdMaya/shadingModeRegistry.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/usdGeom/tokens.h"

#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MNodeClass.h>
#include <maya/MTypeId.h>

#include <ostream>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE



TF_DEFINE_PUBLIC_TOKENS(PxrUsdMayaTranslatorTokens,
        PXRUSDMAYA_TRANSLATOR_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(PxrUsdExportJobArgsTokens, 
        PXRUSDMAYA_JOBEXPORTARGS_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(PxrUsdImportJobArgsTokens, 
        PXRUSDMAYA_JOBIMPORTARGS_TOKENS);

TF_DEFINE_PRIVATE_TOKENS(
    _usdExportInfoScope,

    (UsdMaya)
    (UsdExport)
);

TF_DEFINE_PRIVATE_TOKENS(
    _usdImportInfoScope,

    (UsdMaya)
    (UsdImport)
);

/// Extracts a bool at \p key from \p userArgs, or false if it can't extract.
static bool
_Boolean(const VtDictionary& userArgs, const TfToken& key)
{
    if (!VtDictionaryIsHolding<bool>(userArgs, key)) {
        TF_CODING_ERROR("Dictionary is missing required key '%s' or key is "
                "not bool type", key.GetText());
        return false;
    }
    return VtDictionaryGet<bool>(userArgs, key);
}

/// Extracts a string at \p key from \p userArgs, or "" if it can't extract.
static std::string
_String(const VtDictionary& userArgs, const TfToken& key)
{
    if (!VtDictionaryIsHolding<std::string>(userArgs, key)) {
        TF_CODING_ERROR("Dictionary is missing required key '%s' or key is "
                "not string type", key.GetText());
        return std::string();
    }
    return VtDictionaryGet<std::string>(userArgs, key);
}

/// Extracts a token at \p key from \p userArgs.
/// If the token value is not either \p defaultToken or one of the
/// \p otherTokens, then returns \p defaultToken instead.
static TfToken
_Token(
    const VtDictionary& userArgs,
    const TfToken& key,
    const TfToken& defaultToken,
    const std::vector<TfToken>& otherTokens)
{
    const TfToken tok(_String(userArgs, key));
    for (const TfToken& allowedTok : otherTokens) {
        if (tok == allowedTok) {
            return tok;
        }
    }

    // Empty token will silently be promoted to default value.
    // Warning for non-empty tokens that don't match.
    if (tok != defaultToken && !tok.IsEmpty()) {
        TF_WARN("Value '%s' is not allowed for flag '%s'; using fallback '%s' "
                "instead",
                tok.GetText(), key.GetText(), defaultToken.GetText());
    }
    return defaultToken;
}

/// Extracts an absolute path at \p key from \p userArgs, or the empty path if
/// it can't extract.
static SdfPath
_AbsolutePath(const VtDictionary& userArgs, const TfToken& key)
{
    const std::string s = _String(userArgs, key);
    // Assume that empty strings are empty paths. (This might be an error case.)
    if (s.empty()) {
        return SdfPath();
    }
    // Make all relative paths into absolute paths.
    SdfPath path(s);
    if (path.IsAbsolutePath()) {
        return path;
    }
    else {
        return SdfPath::AbsoluteRootPath().AppendPath(path);
    }
}

/// Extracts an vector<T> from the vector<VtValue> at \p key in \p userArgs.
/// Returns an empty vector if it can't convert the entire value at \p key into
/// a vector<T>. 
template <typename T>
static std::vector<T>
_Vector(const VtDictionary& userArgs, const TfToken& key)
{
    // Check that vector exists.
    if (!VtDictionaryIsHolding<std::vector<VtValue>>(userArgs, key)) {
        TF_CODING_ERROR("Dictionary is missing required key '%s' or key is "
                "not vector type", key.GetText());
        return std::vector<T>();
    }

    // Check that vector is correctly-typed.
    std::vector<VtValue> vals =
            VtDictionaryGet<std::vector<VtValue>>(userArgs, key);
    if (!std::all_of(vals.begin(), vals.end(),
            [](const VtValue& v) { return v.IsHolding<T>(); })) {
        TF_CODING_ERROR("Vector at dictionary key '%s' contains elements of "
                "the wrong type", key.GetText());
        return std::vector<T>();
    }

    // Extract values.
    std::vector<T> result;
    for (const VtValue& v : vals) {
        result.push_back(v.UncheckedGet<T>());
    }
    return result;
}

/// Convenience function that takes the result of _Vector and converts it to a
/// TfToken::Set.
static TfToken::Set
_TokenSet(const VtDictionary& userArgs, const TfToken& key)
{
    const std::vector<std::string> vec = _Vector<std::string>(userArgs, key);
    TfToken::Set result;
    for (const std::string& s : vec) {
        result.insert(TfToken(s));
    }
    return result;
}

// The chaser args are stored as vectors of vectors (since this is how you
// would need to pass them in the Maya Python command API). Convert this to a
// map of maps.
static std::map<std::string, PxrUsdMayaJobExportArgs::ChaserArgs>
_ChaserArgs(const VtDictionary& userArgs, const TfToken& key)
{
    const std::vector<std::vector<VtValue>> chaserArgs =
            _Vector<std::vector<VtValue>>(userArgs, key);

    std::map<std::string, PxrUsdMayaJobExportArgs::ChaserArgs> result;
    for (const std::vector<VtValue>& argTriple : chaserArgs) {
        if (argTriple.size() != 3) {
            TF_CODING_ERROR(
                    "Each chaser arg must be a triple (chaser, arg, value)");
            return std::map<std::string, PxrUsdMayaJobExportArgs::ChaserArgs>();
        }

        const std::string& chaser = argTriple[0].Get<std::string>();
        const std::string& arg = argTriple[1].Get<std::string>();
        const std::string& value = argTriple[2].Get<std::string>();
        result[chaser][arg] = value;
    }
    return result;
}

PxrUsdMayaJobExportArgs::PxrUsdMayaJobExportArgs(
    const VtDictionary& userArgs,
    const PxrUsdMayaUtil::MDagPathSet& dagPaths,
    const GfInterval& timeInterval) :
        defaultMeshScheme(
            _Token(userArgs,
                PxrUsdExportJobArgsTokens->defaultMeshScheme,
                UsdGeomTokens->catmullClark,
                {
                    UsdGeomTokens->loop,
                    UsdGeomTokens->bilinear,
                    UsdGeomTokens->none
                })),
        eulerFilter(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->eulerFilter)),
        excludeInvisible(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->renderableOnly)),
        exportCollectionBasedBindings(
            _Boolean(userArgs,
                PxrUsdExportJobArgsTokens->exportCollectionBasedBindings)),
        exportColorSets(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->exportColorSets)),
        exportDefaultCameras(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->defaultCameras)),
        exportDisplayColor(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->exportDisplayColor)),
        exportInstances(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->exportInstances)),
        exportMaterialCollections(
            _Boolean(userArgs,
                PxrUsdExportJobArgsTokens->exportMaterialCollections)),
        exportMeshUVs(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->exportUVs)),
        exportNurbsExplicitUV(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->exportUVs)),
        exportReferenceObjects(
            _Boolean(userArgs,
                PxrUsdExportJobArgsTokens->exportReferenceObjects)),
        exportRefsAsInstanceable(
            _Boolean(userArgs,
                PxrUsdExportJobArgsTokens->exportRefsAsInstanceable)),
        exportSkels(
            _Token(userArgs,
                PxrUsdExportJobArgsTokens->exportSkels,
                PxrUsdExportJobArgsTokens->none,
                {
                    PxrUsdExportJobArgsTokens->auto_,
                    PxrUsdExportJobArgsTokens->explicit_
                })),
        exportSkin(
            _Token(userArgs,
                PxrUsdExportJobArgsTokens->exportSkin,
                PxrUsdExportJobArgsTokens->none,
                {
                    PxrUsdExportJobArgsTokens->auto_,
                    PxrUsdExportJobArgsTokens->explicit_
                })),
        exportVisibility(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->exportVisibility)),
        materialCollectionsPath(
            _AbsolutePath(userArgs,
                PxrUsdExportJobArgsTokens->materialCollectionsPath)),
        mergeTransformAndShape(
            _Boolean(userArgs,
                PxrUsdExportJobArgsTokens->mergeTransformAndShape)),
        normalizeNurbs(
            _Boolean(userArgs, PxrUsdExportJobArgsTokens->normalizeNurbs)),
        stripNamespaces(
            _Boolean(userArgs,
                PxrUsdExportJobArgsTokens->stripNamespaces)),
        parentScope(
            _AbsolutePath(userArgs, PxrUsdExportJobArgsTokens->parentScope)),
        renderLayerMode(
            _Token(userArgs,
                PxrUsdExportJobArgsTokens->renderLayerMode,
                PxrUsdExportJobArgsTokens->defaultLayer,
                {
                    PxrUsdExportJobArgsTokens->currentLayer,
                    PxrUsdExportJobArgsTokens->modelingVariant
                })),
        rootKind(
            _String(userArgs, PxrUsdExportJobArgsTokens->kind)),
        shadingMode(
            _Token(userArgs,
                PxrUsdExportJobArgsTokens->shadingMode,
                PxrUsdMayaShadingModeTokens->none,
                PxrUsdMayaShadingModeRegistry::ListExporters())),

        chaserNames(
            _Vector<std::string>(userArgs, PxrUsdExportJobArgsTokens->chaser)),
        allChaserArgs(
            _ChaserArgs(userArgs, PxrUsdExportJobArgsTokens->chaserArgs)),

        melPerFrameCallback(
            _String(userArgs, PxrUsdExportJobArgsTokens->melPerFrameCallback)),
        melPostCallback(
            _String(userArgs, PxrUsdExportJobArgsTokens->melPostCallback)),
        pythonPerFrameCallback(
            _String(userArgs,
                PxrUsdExportJobArgsTokens->pythonPerFrameCallback)),
        pythonPostCallback(
            _String(userArgs, PxrUsdExportJobArgsTokens->pythonPostCallback)),

        dagPaths(dagPaths),
        timeInterval(timeInterval)
{
}

std::ostream&
operator <<(std::ostream& out, const PxrUsdMayaJobExportArgs& exportArgs)
{
    out << "exportRefsAsInstanceable: " << TfStringify(exportArgs.exportRefsAsInstanceable) << std::endl
        << "exportDisplayColor: " << TfStringify(exportArgs.exportDisplayColor) << std::endl
        << "shadingMode: " << exportArgs.shadingMode << std::endl
        << "mergeTransformAndShape: " << TfStringify(exportArgs.mergeTransformAndShape) << std::endl
        << "exportInstances: " << TfStringify(exportArgs.exportInstances) << std::endl
        << "timeInterval: " << exportArgs.timeInterval << std::endl
        << "eulerFilter: " << TfStringify(exportArgs.eulerFilter) << std::endl
        << "excludeInvisible: " << TfStringify(exportArgs.excludeInvisible) << std::endl
        << "exportDefaultCameras: " << TfStringify(exportArgs.exportDefaultCameras) << std::endl
        << "exportSkels: " << TfStringify(exportArgs.exportSkels) << std::endl
        << "exportSkin: " << TfStringify(exportArgs.exportSkin) << std::endl
        << "exportMeshUVs: " << TfStringify(exportArgs.exportMeshUVs) << std::endl
        << "exportMaterialCollections: " << TfStringify(exportArgs.exportMaterialCollections) << std::endl
        << "materialCollectionsPath: " << exportArgs.materialCollectionsPath << std::endl
        << "exportCollectionBasedBindings: " << TfStringify(exportArgs.exportCollectionBasedBindings) << std::endl
        << "normalizeNurbs: " << TfStringify(exportArgs.normalizeNurbs) << std::endl
        << "exportNurbsExplicitUV: " << TfStringify(exportArgs.exportNurbsExplicitUV) << std::endl
        << "exportColorSets: " << TfStringify(exportArgs.exportColorSets) << std::endl
        << "renderLayerMode: " << exportArgs.renderLayerMode << std::endl
        << "defaultMeshScheme: " << exportArgs.defaultMeshScheme << std::endl
        << "exportVisibility: " << TfStringify(exportArgs.exportVisibility) << std::endl
        << "stripNamespaces: " << TfStringify(exportArgs.stripNamespaces) << std::endl
        << "parentScope: " << exportArgs.parentScope << std::endl;

    out << "melPerFrameCallback: " << exportArgs.melPerFrameCallback << std::endl
        << "melPostCallback: " << exportArgs.melPostCallback << std::endl
        << "pythonPerFrameCallback: " << exportArgs.pythonPerFrameCallback << std::endl
        << "pythonPostCallback: " << exportArgs.pythonPostCallback << std::endl;

    out << "dagPaths (" << exportArgs.dagPaths.size() << ")" << std::endl;
    for (const MDagPath& dagPath : exportArgs.dagPaths) {
        out << "    " << dagPath.fullPathName().asChar() << std::endl;
    }
    out << "_filteredTypeIds (" << exportArgs.GetFilteredTypeIds().size() << ")" << std::endl;
    for (unsigned int id : exportArgs.GetFilteredTypeIds()) {
        out << "    " << id << ": " << MNodeClass(MTypeId(id)).className() << std::endl;
    }

    out << "chaserNames (" << exportArgs.chaserNames.size() << ")" << std::endl;
    for (const std::string& chaserName : exportArgs.chaserNames) {
        out << "    " << chaserName << std::endl;        
    }

    out << "allChaserArgs (" << exportArgs.allChaserArgs.size() << ")" << std::endl;
    for (const auto& chaserIter : exportArgs.allChaserArgs) {
        // Chaser name.
        out << "    " << chaserIter.first << std::endl;

        for (const auto& argIter : chaserIter.second) {
            out << "        Arg Name: " << argIter.first
                << ", Value: " << argIter.second << std::endl;
        }
    }

    out << "usdModelRootOverridePath: " << exportArgs.usdModelRootOverridePath << std::endl
        << "rootKind: " << exportArgs.rootKind << std::endl;

    return out;
}

/* static */
PxrUsdMayaJobExportArgs PxrUsdMayaJobExportArgs::CreateFromDictionary(
    const VtDictionary& userArgs,
    const PxrUsdMayaUtil::MDagPathSet& dagPaths,
    const GfInterval& timeInterval)
{
    return PxrUsdMayaJobExportArgs(
            VtDictionaryOver(userArgs, GetDefaultDictionary()),
            dagPaths,
            timeInterval);
}

/* static */
const VtDictionary& PxrUsdMayaJobExportArgs::GetDefaultDictionary()
{
    static VtDictionary d;
    static std::once_flag once;
    std::call_once(once, []() {
        // Base defaults.
        d[PxrUsdExportJobArgsTokens->chaser] = std::vector<VtValue>();
        d[PxrUsdExportJobArgsTokens->chaserArgs] = std::vector<VtValue>();
        d[PxrUsdExportJobArgsTokens->defaultCameras] = false;
        d[PxrUsdExportJobArgsTokens->defaultMeshScheme] = 
                UsdGeomTokens->catmullClark.GetString();
        d[PxrUsdExportJobArgsTokens->exportCollectionBasedBindings] = false;
        d[PxrUsdExportJobArgsTokens->exportColorSets] = true;
        d[PxrUsdExportJobArgsTokens->exportDisplayColor] = true;
        d[PxrUsdExportJobArgsTokens->exportInstances] = true;
        d[PxrUsdExportJobArgsTokens->exportMaterialCollections] = false;
        d[PxrUsdExportJobArgsTokens->exportReferenceObjects] = false;
        d[PxrUsdExportJobArgsTokens->exportRefsAsInstanceable] = false;
        d[PxrUsdExportJobArgsTokens->exportSkin] =
                PxrUsdExportJobArgsTokens->none.GetString();
        d[PxrUsdExportJobArgsTokens->exportSkels] =
                PxrUsdExportJobArgsTokens->none.GetString();
        
        d[PxrUsdExportJobArgsTokens->exportUVs] = true;
        d[PxrUsdExportJobArgsTokens->exportVisibility] = true;
        d[PxrUsdExportJobArgsTokens->kind] = std::string();
        d[PxrUsdExportJobArgsTokens->materialCollectionsPath] = std::string();
        d[PxrUsdExportJobArgsTokens->melPerFrameCallback] = std::string();
        d[PxrUsdExportJobArgsTokens->melPostCallback] = std::string();
        d[PxrUsdExportJobArgsTokens->mergeTransformAndShape] = true;
        d[PxrUsdExportJobArgsTokens->normalizeNurbs] = false;
        d[PxrUsdExportJobArgsTokens->parentScope] = std::string();
        d[PxrUsdExportJobArgsTokens->pythonPerFrameCallback] = std::string();
        d[PxrUsdExportJobArgsTokens->pythonPostCallback] = std::string();
        d[PxrUsdExportJobArgsTokens->renderableOnly] = false;
        d[PxrUsdExportJobArgsTokens->renderLayerMode] =
                PxrUsdExportJobArgsTokens->defaultLayer.GetString();
        d[PxrUsdExportJobArgsTokens->shadingMode] =
                PxrUsdMayaShadingModeTokens->displayColor.GetString();
        d[PxrUsdExportJobArgsTokens->stripNamespaces] = false;
        d[PxrUsdExportJobArgsTokens->eulerFilter] = false;

        // plugInfo.json site defaults.
        // The defaults dict should be correctly-typed, so enable
        // coerceToWeakerOpinionType.
        const VtDictionary site =
                PxrUsdMaya_RegistryHelper::GetComposedInfoDictionary(
                _usdExportInfoScope->allTokens);
        VtDictionaryOver(site, &d, /*coerceToWeakerOpinionType*/ true);
    });

    return d;
}

void PxrUsdMayaJobExportArgs::AddFilteredTypeName(const MString& typeName)
{
    MNodeClass cls(typeName);
    unsigned int id = cls.typeId().id();
    if (id == 0) {
        TF_WARN("Given excluded node type '%s' does not exist; ignoring",
                typeName.asChar());
        return;
    }
    _filteredTypeIds.insert(id);
    // We also insert all inherited types - only way to query this is through mel,
    // which is slower, but this should be ok, as these queries are only done
    // "up front" when the export starts, not per-node
    MString queryCommand("nodeType -isTypeName -derived ");
    queryCommand += typeName;
    MStringArray inheritedTypes;
    MStatus status = MGlobal::executeCommand(queryCommand, inheritedTypes, false, false);
    if (!status) {
        TF_WARN("Error querying derived types for '%s': %s",
                typeName.asChar(), status.errorString().asChar());
        return;
    }

    for (unsigned int i=0; i < inheritedTypes.length(); ++i) {
        if (inheritedTypes[i].length() == 0) continue;
        id = MNodeClass(inheritedTypes[i]).typeId().id();
        if (id == 0) {
            // Unfortunately, the returned list will often include weird garbage, like
            // "THconstraint" for "constraint", which cannot be converted to a MNodeClass,
            // so just ignore these...
            continue;
        }
        _filteredTypeIds.insert(id);
    }
}

PxrUsdMayaJobImportArgs::PxrUsdMayaJobImportArgs(
    const VtDictionary& userArgs,
    const bool importWithProxyShapes,
    const GfInterval& timeInterval) :
        assemblyRep(
            _Token(userArgs,
                PxrUsdImportJobArgsTokens->assemblyRep,
                PxrUsdImportJobArgsTokens->Collapsed,
                {
                    PxrUsdImportJobArgsTokens->Full,
                    PxrUsdImportJobArgsTokens->Import,
                    PxrUsdImportJobArgsTokens->Unloaded
                })),
        excludePrimvarNames(
            _TokenSet(userArgs, PxrUsdImportJobArgsTokens->excludePrimvar)),
        includeAPINames(
            _TokenSet(userArgs, PxrUsdImportJobArgsTokens->apiSchema)),
        includeMetadataKeys(
            _TokenSet(userArgs, PxrUsdImportJobArgsTokens->metadata)),
        shadingMode(
            _Token(userArgs,
                PxrUsdImportJobArgsTokens->shadingMode,
                PxrUsdMayaShadingModeTokens->none,
                PxrUsdMayaShadingModeRegistry::ListImporters())),
        useAsAnimationCache(
            _Boolean(userArgs,
                PxrUsdImportJobArgsTokens->useAsAnimationCache)),

        importWithProxyShapes(importWithProxyShapes),
        timeInterval(timeInterval)
{
}

/* static */
PxrUsdMayaJobImportArgs PxrUsdMayaJobImportArgs::CreateFromDictionary(
    const VtDictionary& userArgs,
    const bool importWithProxyShapes,
    const GfInterval& timeInterval)
{
    return PxrUsdMayaJobImportArgs(
            VtDictionaryOver(userArgs, GetDefaultDictionary()),
            importWithProxyShapes,
            timeInterval);
}

/* static */
const VtDictionary& PxrUsdMayaJobImportArgs::GetDefaultDictionary()
{
    static VtDictionary d;
    static std::once_flag once;
    std::call_once(once, []() {
        // Base defaults.
        d[PxrUsdImportJobArgsTokens->assemblyRep] =
                PxrUsdImportJobArgsTokens->Collapsed.GetString();
        d[PxrUsdImportJobArgsTokens->apiSchema] = std::vector<VtValue>();
        d[PxrUsdImportJobArgsTokens->excludePrimvar] = std::vector<VtValue>();
        d[PxrUsdImportJobArgsTokens->metadata] =
                std::vector<VtValue>({
                    VtValue(SdfFieldKeys->Hidden.GetString()),
                    VtValue(SdfFieldKeys->Instanceable.GetString()),
                    VtValue(SdfFieldKeys->Kind.GetString())
                });
        d[PxrUsdImportJobArgsTokens->shadingMode] =
                PxrUsdMayaShadingModeTokens->displayColor.GetString();
        d[PxrUsdImportJobArgsTokens->useAsAnimationCache] = false;

        // plugInfo.json site defaults.
        // The defaults dict should be correctly-typed, so enable
        // coerceToWeakerOpinionType.
        const VtDictionary site =
                PxrUsdMaya_RegistryHelper::GetComposedInfoDictionary(
                _usdImportInfoScope->allTokens);
        VtDictionaryOver(site, &d, /*coerceToWeakerOpinionType*/ true);
    });

    return d;
}

std::ostream&
operator <<(std::ostream& out, const PxrUsdMayaJobImportArgs& importArgs)
{
    out << "shadingMode: " << importArgs.shadingMode << std::endl
        << "assemblyRep: " << importArgs.assemblyRep << std::endl
        << "timeInterval: " << importArgs.timeInterval << std::endl
        << "useAsAnimationCache: " << TfStringify(importArgs.useAsAnimationCache) << std::endl
        << "importWithProxyShapes: " << TfStringify(importArgs.importWithProxyShapes) << std::endl;

    return out;
}


PXR_NAMESPACE_CLOSE_SCOPE
