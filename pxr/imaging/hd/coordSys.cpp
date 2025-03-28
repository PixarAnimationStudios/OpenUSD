//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
