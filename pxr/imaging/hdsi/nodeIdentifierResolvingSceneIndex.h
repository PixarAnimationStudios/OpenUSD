//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HD_SI_NODE_IDENTIFIER_RESOLVING_SCENE_INDEX_H
#define PXR_IMAGING_HD_SI_NODE_IDENTIFIER_RESOLVING_SCENE_INDEX_H

#include "pxr/imaging/hd/materialFilteringSceneIndexBase.h"
#include "pxr/imaging/hdsi/api.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdSiNodeIdentifierResolvingSceneIndex);

/// Scene index that converts the <sourceType>:sourceAsset info into a 
/// nodeType (nodeIdentifier).
class HdSiNodeIdentifierResolvingSceneIndex
    : public HdMaterialFilteringSceneIndexBase
{
public:

    /// Construct a new instance of HdSiNodeIdentifierResolvingSceneIndex.
    /// \p sourceType indicates the type of the shader's source or its 
    /// implementation, e.g. OSL, glslfx, riCpp etc. . . 
    /// See also: UsdShadeNodeDefAPI for more details about sourceType.
    HDSI_API
    static
    HdSiNodeIdentifierResolvingSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex, 
        const TfToken &sourceType);

    HDSI_API
    ~HdSiNodeIdentifierResolvingSceneIndex() override;

protected: // HdMaterialFilteringSceneIndexBase overrides
    HDSI_API
    FilteringFnc _GetFilteringFunction() const override;

private:
    HdSiNodeIdentifierResolvingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        const TfToken &sourceType);

    TfToken _sourceType;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
