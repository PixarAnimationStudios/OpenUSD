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
#include "ModelKindWriter.h"

#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/kind/registry.h"

PxrUsdMaya_ModelKindWriter::PxrUsdMaya_ModelKindWriter(
    const JobExportArgs& args)
    : _args(args),
      _rootIsAssembly(KindRegistry::IsA(args.rootKind, KindTokens->assembly))
{
}

void
PxrUsdMaya_ModelKindWriter::OnWritePrim(
    const UsdPrim& prim,
    const MayaPrimWriterPtr& primWriter)
{
    SdfPath path = prim.GetPath();

    // Here we save all of the root prims that are assemblies (or derived), so
    // that we can show error messages indicating that there are gprims under
    // a prim with kind=assembly.
    if (path.IsRootPrimPath()) {
        TfToken kind;
        UsdModelAPI(prim).GetKind(&kind);
        if (_rootIsAssembly || KindRegistry::IsA(kind, KindTokens->assembly)) {
            _pathsToExportedGprimsMap.emplace(path, std::vector<SdfPath>());
        }
    }

    // If exporting a gprim, place the root prim in the set of root prims with
    // gprims. Then record the actual gprim if its root prim has been tagged as
    // potentially an assembly.
    if (primWriter->exportsGprims()) {
        auto pathPrefixes = path.GetPrefixes();
        SdfPath rootPath;
        if (pathPrefixes.size() > 0) {
            rootPath = path.GetPrefixes()[0];
        } else {
            rootPath = path;
        }

        _pathsWithExportedGprims.insert(rootPath);

        auto iter = _pathsToExportedGprimsMap.find(rootPath);
        if (iter != _pathsToExportedGprimsMap.end()) {
            std::vector<SdfPath>& paths = iter->second;
            paths.push_back(path);
        }
    }

    if (primWriter->exportsReferences()) {
        _pathsThatMayHaveKind.push_back(prim.GetPath());
    }
}

bool
PxrUsdMaya_ModelKindWriter::MakeModelHierarchy(UsdStageRefPtr& stage)
{
    // For any root-prim that doesn't already have an authored kind 
    // (thinking ahead to being able to specify USD_kind per bug/128430),
    // make it a model.  If there were any gprims authored directly during
    // export, we will make the roots be component models, and author
    // kind=subcomponent on any prim-references that would otherwise
    // evaluate to some model-kind; we may in future make this behavior
    // a jobargs option.
    //
    // If there were no gprims directly authored, we'll make it an assembly 
    // instead, and attempt to create a valid model-hierarchy if any of the
    // references we authored are references to models.
    //
    // Note that the code below does its best to facilitate having multiple,
    // independent root-trees/models in the same export, however the
    // analysis we have done about gprims and references authored is global,
    // so all trees will get the same treatment/kind.

    SdfPathBoolMap rootPrimIsComponent;

    // One pass through root prims to fill in root-kinds.
    if (!_AuthorRootPrimKinds(stage, rootPrimIsComponent)) {
        return false;
    }

    if (!_FixUpPrimKinds(stage, rootPrimIsComponent)) {
        return false;
    }

    return true;
}

bool
PxrUsdMaya_ModelKindWriter::_AuthorRootPrimKinds(
    UsdStageRefPtr& stage,
    SdfPathBoolMap& rootPrimIsComponent)
{
    UsdPrimSiblingRange usdRootPrims = stage->GetPseudoRoot().GetChildren();
    for (UsdPrim const& prim : usdRootPrims) {
        SdfPath primPath = prim.GetPath();
        UsdModelAPI usdRootModel(prim);
        TfToken kind;
        usdRootModel.GetKind(&kind);

        // If the rootKind job arg was set, then we need to check it against
        // the existing kind (if any).
        // Empty kinds will be replaced by the rootKind, and incompatible kinds
        // should cause an error.
        // An existing kind that derives from rootKind is acceptable, and will
        // be preserved.
        if (!_args.rootKind.IsEmpty()) {
            if (kind.IsEmpty()) {
                // If no existing kind, author based on rootKind job arg.
                kind = _args.rootKind;
                usdRootModel.SetKind(kind);
            }
            else if (!KindRegistry::IsA(kind, _args.rootKind)) {
                // If existing kind is not derived from rootKind, then error.
                MString errorMsg = primPath.GetText();
                errorMsg += " has kind '";
                errorMsg += kind.GetText();
                errorMsg += "' but the rootKind argument was '";
                errorMsg += _args.rootKind.GetText();
                errorMsg += "'; expected that or derived kind.";
                MGlobal::displayError(errorMsg);
                return false;
            }
        }

        bool hasExportedGprims = false;
        const auto pathIter = _pathsWithExportedGprims.find(primPath);
        if (pathIter != _pathsWithExportedGprims.end()) {
            hasExportedGprims = true;
        }

        if (kind.IsEmpty()) {
            // Author kind based on hasExportedGprims.
            kind = hasExportedGprims ?
                    KindTokens->component : KindTokens->assembly;
            usdRootModel.SetKind(kind);
        } else {
            // Verify kind based on hasExportedGprims.
            if (hasExportedGprims &&
                    KindRegistry::IsA(kind, KindTokens->assembly)) {
                MString errorMsg = primPath.GetText();
                errorMsg += " has kind '";
                errorMsg += kind.GetText();
                errorMsg += "' and cannot have a mesh below. Please remove:";

                const auto exportedGprimsIter =
                        _pathsToExportedGprimsMap.find(primPath);
                if (exportedGprimsIter != _pathsToExportedGprimsMap.end()) {
                    std::vector<SdfPath>& paths = exportedGprimsIter->second;
                    for (const SdfPath& p : paths) {
                        errorMsg += "\n  ";
                        errorMsg += p.GetText();
                    }
                }

                MGlobal::displayError(errorMsg);
                return false;
            }
        }

        rootPrimIsComponent[primPath] =
                KindRegistry::IsA(kind, KindTokens->component);
    }

    return true;
}

bool
PxrUsdMaya_ModelKindWriter::_FixUpPrimKinds(
    UsdStageRefPtr& stage,
    const SdfPathBoolMap& rootPrimIsComponent)
{
    std::unordered_set<SdfPath, SdfPath::Hash> pathsToBeGroup;
    for (SdfPath const &path : _pathsThatMayHaveKind) {
        // The kind of the root prim under which each reference was authored
        // informs how we will fix-up/fill-in kind on it and its ancestors.
        UsdPrim prim = stage->GetPrimAtPath(path);
        if (!prim) {
            continue;
        }
        UsdModelAPI usdModel(prim);
        TfToken kind;
        
        // Nothing to fix if there's no resolved kind.
        if (!usdModel.GetKind(&kind) || kind.IsEmpty()) {
            continue;
        }
        
        SdfPathVector ancestorPaths;
        path.GetParentPath().GetPrefixes(&ancestorPaths);
        if (ancestorPaths.size() < 1) {
            continue;
        }

        auto iter = rootPrimIsComponent.find(ancestorPaths[0]);
        if (iter != rootPrimIsComponent.end() && iter->second) {
            // Override any authored kind below the root to subcomponent
            // to avoid broken model-hierarchy.
            usdModel.SetKind(KindTokens->subcomponent);
        } else {
            // Just insert the paths into a set at this point so that we
            // can do the authoring in batch Sdf API for efficiency.
            for (size_t i = 1; i < ancestorPaths.size(); ++i) {
                UsdPrim ancestorPrim = stage->GetPrimAtPath(ancestorPaths[i]);
                if (!ancestorPrim)
                    continue;
                UsdModelAPI ancestorModel(ancestorPrim);
                TfToken  kind;
                
                if (!usdModel.GetKind(&kind) || 
                    !KindRegistry::IsA(kind, KindTokens->group)) {
                    pathsToBeGroup.insert(ancestorPaths[i]);
                }
            }
        }
    }

    {        
        // We drop down to Sdf to do the kind-authoring, because authoring
        // kind induces recomposition since we cache model-hierarchy.  Using
        // Sdf api, we can bundle the changes into a change block, and do all
        // the recomposition at once
        SdfLayerHandle layer = stage->GetEditTarget().GetLayer();
        SdfChangeBlock block;

        for (SdfPath const &path : pathsToBeGroup) {
            SdfPrimSpecHandle primSpec = SdfCreatePrimInLayer(layer, path);
            if (!primSpec) {
                MGlobal::displayError("Failed to create primSpec for setting "
                        "kind at path:" + MString(path.GetText()));
            }
            else {
                primSpec->SetKind(KindTokens->group);
            }
        }       
    }

    return true;
}

void
PxrUsdMaya_ModelKindWriter::Reset()
{
    _pathsThatMayHaveKind.clear();
    _pathsToExportedGprimsMap.clear();
    _pathsWithExportedGprims.clear();
}
