//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_SUB_LAYER_LIST_EDITOR_H
#define PXR_USD_SDF_SUB_LAYER_LIST_EDITOR_H

/// \file sdf/subLayerListEditor.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/vectorListEditor.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/proxyPolicies.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// \class Sdf_SubLayerListEditor
///
/// List editor implementation for sublayer path lists.
///
class Sdf_SubLayerListEditor 
    : public Sdf_VectorListEditor<SdfSubLayerTypePolicy>
{
public:
    Sdf_SubLayerListEditor(const SdfLayerHandle& owner);

    virtual ~Sdf_SubLayerListEditor();

private:
    typedef Sdf_VectorListEditor<SdfSubLayerTypePolicy> Parent;

    virtual void _OnEdit(
        SdfListOpType op,
        const std::vector<std::string>& oldValues,
        const std::vector<std::string>& newValues) const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_SUB_LAYER_LIST_EDITOR_H
