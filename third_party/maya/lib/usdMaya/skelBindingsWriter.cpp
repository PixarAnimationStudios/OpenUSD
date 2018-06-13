//
// Copyright 2018 Pixar
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
#include "usdMaya/skelBindingsWriter.h"

#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdUtils/authoring.h"

#include "usdMaya/translatorUtil.h"

PXR_NAMESPACE_OPEN_SCOPE

PxrUsdMaya_SkelBindingsWriter::PxrUsdMaya_SkelBindingsWriter()
{
}

/// Finds the rootmost ancestor of the prim at \p path that is an Xform
/// or SkelRoot type prim. The result may be the prim itself.
static UsdPrim
_FindRootmostXformOrSkelRoot(const UsdStagePtr& stage, const SdfPath& path)
{
    UsdPrim currentPrim = stage->GetPrimAtPath(path);
    UsdPrim rootmost;
    while (currentPrim) {
        if (currentPrim.IsA<UsdGeomXform>()) {
            rootmost = currentPrim;
        } else if (currentPrim.IsA<UsdSkelRoot>()) {
            rootmost = currentPrim;
        }
        currentPrim = currentPrim.GetParent();
    }

    return rootmost;
}


/// Finds the existing SkelRoot which is shared by all \p paths.
/// If no SkelRoot is found, and \p config is "auto", then attempts to
/// find a common ancestor of \p paths which can be converted to SkelRoot.
/// \p outMadeSkelRoot must be non-null; it will be set to indicate whether
/// any auto-typename change actually occurred (true) or whether there was
/// already a SkelRoot, so no renaming was necessary (false).
/// If an existing, common SkelRoot cannot be found for all paths, and if
/// it's not possible to create one, returns an empty SdfPath.
static SdfPath
_VerifyOrMakeSkelRoot(const UsdStagePtr& stage,
                      const SdfPathVector& paths,
                      const TfToken& config)
{
    if ((config != PxrUsdExportJobArgsTokens->auto_ &&
         config != PxrUsdExportJobArgsTokens->explicit_) || paths.size() == 0) {
        return SdfPath();
    }

    SdfPath firstPath = paths.front();

    // Only try to auto-rename to SkelRoot if we're not already a
    // descendant of one. Otherwise, verify that the user tagged it in a sane
    // way.

    if (UsdSkelRoot root = UsdSkelRoot::Find(stage->GetPrimAtPath(firstPath))) {

        // Verify that all other paths being considered are encapsulated
        // within the same skel root.
        for (size_t i = 1; i < paths.size(); ++i) {
            if (UsdSkelRoot root2 =
                UsdSkelRoot::Find(stage->GetPrimAtPath(paths[i]))) {

                if (root2.GetPrim() != root.GetPrim()) {
                    MGlobal::displayError(
                        TfStringPrintf(
                            "Expected SkelRoot for prim <%s> to be under the "
                            "same SkelRoot as prim <%s> (<%s>), but instead "
                            "found <%s>. This might cause unexpected "
                            "behavior.", paths[i].GetText(),
                            firstPath.GetText(),
                            root.GetPrim().GetPath().GetText(),
                            root2.GetPrim().GetPath().GetText()).c_str());
                    return SdfPath();
                }
            } else {
                MGlobal::displayError(
                    TfStringPrintf(
                        "Expected SkelRoot for prim <%s> to be under the same "
                        "SkelRoot as prim <%s> (%s), but it is not under a "
                        "SkelRoot at all. This might cause unexpected "
                        "behavior.", paths[i].GetText(),
                        firstPath.GetText(),
                        root.GetPrim().GetPath().GetText()).c_str());
                return SdfPath();
            }
        }
        

        // Verify that the SkelRoot isn't nested in another SkelRoot.
        // This is necessary because UsdSkel doesn't handle nested skel roots
        // very well currently; this restriction may be loosened in the future.
        if (UsdSkelRoot root2 = UsdSkelRoot::Find(root.GetPrim().GetParent())) {
            MGlobal::displayError(TfStringPrintf("The SkelRoot <%s> is nested "
                    "inside another SkelRoot <%s>. This might cause unexpected "
                    "behavior. ",
                    root.GetPath().GetText(),
                    root2.GetPath().GetText()).c_str());
            return SdfPath();
        }
        else {
            return root.GetPath();
        }
    } else if(config == PxrUsdExportJobArgsTokens->auto_) {
        // If auto-generating the SkelRoot, find the rootmost
        // UsdGeomXform and turn it into a SkelRoot.
        // XXX: It might be good to also consider model hierarchy here, and not
        // go past our ancestor component when trying to generate the SkelRoot.
        // (Example: in a scene with /World, /World/Char_1, /World/Char_2, we
        // might want SkelRoots to stop at Char_1 and Char_2.) Unfortunately,
        // the current structure precludes us from accessing model hierarchy
        // here.
        if (UsdPrim root = _FindRootmostXformOrSkelRoot(stage, firstPath)) {

            for (size_t i = 1; i < paths.size(); ++i) {
                if (!paths[i].HasPrefix(root.GetPath())) {
                    MGlobal::displayError(
                        TfStringPrintf(
                            "Could not find a common ancestor of prim <%s> and "
                            "<%s> that can be converted to a SkelRoot. "
                            "Try giving the primitives a common, transform "
                            "ancestor node.", firstPath.GetText(),
                            paths[i].GetText()).c_str());
                    return SdfPath();
                }
            }

            UsdSkelRoot::Define(stage, root.GetPath());
            return root.GetPath();
        }
        else {
            MGlobal::displayError(
                TfStringPrintf(
                    "Could not find a UsdGeomXform or ancestor of "
                    "prim <%s> that can be converted to a SkelRoot.",
                    firstPath.GetText()).c_str());
            return SdfPath();
        }
    }
    return SdfPath();
}



void
PxrUsdMaya_SkelBindingsWriter::MarkBindings(
    const SdfPath& path,
    const SdfPath& skelInstancePath,
    const TfToken& config)
{
    _bindingToInstanceMap[path] = _Entry(skelInstancePath, config);
}


bool
PxrUsdMaya_SkelBindingsWriter::_VerifyOrMakeSkelRoots(
    const UsdStagePtr& stage) const
{
    bool success = true;
    for (const auto& pair : _bindingToInstanceMap) {
        const _Entry& entry = pair.second;
        SdfPath skelRootPath =
            _VerifyOrMakeSkelRoot(stage, {pair.first, entry.first},
                                  entry.second);
        success &= !skelRootPath.IsEmpty();
    }
    return success;
}


bool
PxrUsdMaya_SkelBindingsWriter::PostProcessSkelBindings(
    const UsdStagePtr& stage) const
{
    bool success = _VerifyOrMakeSkelRoots(stage);

    // TODO: Write extent on all SkelRoot prims with marked bindings.
    // Would like UsdSkel to provide some helper functionality in order
    // to simplify this.
    return success;
}


PXR_NAMESPACE_CLOSE_SCOPE
