//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HD_ST_NODE_IDENTIFIER_RESOLVING_SCENE_INDEX_H
#define PXR_IMAGING_HD_ST_NODE_IDENTIFIER_RESOLVING_SCENE_INDEX_H

#include "pxr/imaging/hd/materialFilteringSceneIndexBase.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdSt_NodeIdentifierResolvingSceneIndex);

/// Scene index that converts the glslfx:sourceAsset info into a nodeType
/// (nodeIdentifier).
class HdSt_NodeIdentifierResolvingSceneIndex
    : public HdMaterialFilteringSceneIndexBase
{
public:
    static
    HdSt_NodeIdentifierResolvingSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex);

    ~HdSt_NodeIdentifierResolvingSceneIndex() override;

protected: // HdMaterialFilteringSceneIndexBase overrides
    FilteringFnc _GetFilteringFunction() const override;

private:
    HdSt_NodeIdentifierResolvingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
