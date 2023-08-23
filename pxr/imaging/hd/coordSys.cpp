//
// Copyright 2019 Pixar
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
#include "pxr/imaging/hd/coordSys.h"

#include "pxr/imaging/hd/coordSysSchema.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (coordSys) 
    (binding)
);

static
TfToken _GetNameFromSdfPath(SdfPath const &path)
{
    const std::string &attrName = path.GetName();
    const std::string &nameSpacedCoordSysName =
        TfStringEndsWith(attrName, _tokens->binding.GetString())
        ? TfStringGetBeforeSuffix(
                attrName, *SdfPathTokens->namespaceDelimiter.GetText())
        : attrName;
    return TfToken(SdfPath::StripPrefixNamespace(
                nameSpacedCoordSysName, _tokens->coordSys).first);
}

HdCoordSys::HdCoordSys(SdfPath const &id)
 : HdSprim(id)
// Initialize here even though _name is set in Sync.
// We are transitioning to providing the name explicitly rather than through
// the prim name. This initialization is in support for old scene delegates
// not setting the name explicitly and not dirtying in time to make
// sure _name is synced by the time the render delegate calls GetName().
// This is to make testUsdImagingDelegateChanges pass which inspects
// HdCoordSys::GetName() without syncing the render index.
 , _name(_GetNameFromSdfPath(id))
{
}

void
HdCoordSys::Sync(HdSceneDelegate * const sceneDelegate,
                 HdRenderParam   * const renderParam,
                 HdDirtyBits     * const dirtyBits)
{
    TF_UNUSED(renderParam);
    const SdfPath &id = GetId();
    if (!TF_VERIFY(sceneDelegate)) {
        return;
    }

    HdDirtyBits bits = *dirtyBits;

    if (bits & DirtyName) {
        static const TfToken key(
            SdfPath::JoinIdentifier(
                TfTokenVector{ HdCoordSysSchema::GetSchemaToken(),
                               HdCoordSysSchemaTokens->name }));

        const VtValue vName =
            sceneDelegate->Get(
                id, key);
        if (vName.IsHolding<TfToken>()) {
            _name = vName.UncheckedGet<TfToken>();
        } else {
            _name = _GetNameFromSdfPath(id);
        }
    }

    *dirtyBits = Clean;
}

HdCoordSys::~HdCoordSys() = default;

HdDirtyBits
HdCoordSys::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE
