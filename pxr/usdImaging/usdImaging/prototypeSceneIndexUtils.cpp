//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/token.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

static const TfToken::Set primTypeWhitelist {
    HdPrimTypeTokens->material
};

static const TfToken emptyToken;

namespace UsdImaging_PrototypeSceneIndexUtils {

HdSceneIndexPrim&
SetEmptyPrimType(HdSceneIndexPrim& prim)
{
    if (primTypeWhitelist.count(prim.primType) == 0) {
        prim.primType = emptyToken;
    }
    return prim;
}

HdSceneIndexObserver::AddedPrimEntry&
SetEmptyPrimType(HdSceneIndexObserver::AddedPrimEntry& entry) {
    if (primTypeWhitelist.count(entry.primType) == 0) {
        entry.primType = emptyToken;
    }
    return entry;
}

};

PXR_NAMESPACE_CLOSE_SCOPE
