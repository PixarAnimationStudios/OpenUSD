//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define< SdfNotice::Base,
                    TfType::Bases< TfNotice > >();
    TfType::Define< SdfNotice::LayersDidChange,
                    TfType::Bases< SdfNotice::Base > >();
    TfType::Define< SdfNotice::LayersDidChangeSentPerLayer,
                    TfType::Bases< SdfNotice::Base > >();
    TfType::Define< SdfNotice::LayerInfoDidChange,
                    TfType::Bases< SdfNotice::Base > >();
    TfType::Define< SdfNotice::LayerIdentifierDidChange,
                    TfType::Bases< SdfNotice::Base > >();
    TfType::Define< SdfNotice::LayerDidReplaceContent,
                    TfType::Bases< SdfNotice::Base > >();
    TfType::Define< SdfNotice::LayerDidReloadContent,
                    TfType::Bases< SdfNotice::LayerDidReplaceContent > >();
    TfType::Define< SdfNotice::LayerDidSaveLayerToFile,
                    TfType::Bases< SdfNotice::Base > >();
    TfType::Define< SdfNotice::LayerDirtinessChanged,
                    TfType::Bases< SdfNotice::Base > >();
    TfType::Define< SdfNotice::LayerMutenessChanged,
                    TfType::Bases< SdfNotice::Base > >();
}

SdfLayerHandleVector
SdfNotice::BaseLayersDidChange::GetLayers() const
{
    SdfLayerHandleVector layers;
    layers.reserve(_vec->size());
    TF_FOR_ALL(i, *_vec) {
        // XXX:bug 20833 It should be ok to return expired layers here.
        if (i->first)
            layers.push_back(i->first);
    }
    return layers;
}

SdfNotice::LayerIdentifierDidChange::LayerIdentifierDidChange(
    const std::string& oldIdentifier, const std::string& newIdentifier) :
      _oldId(oldIdentifier),
      _newId(newIdentifier)
{
}
    
SdfNotice::Base::~Base() { }
SdfNotice::LayersDidChange::~LayersDidChange() { }
SdfNotice::LayersDidChangeSentPerLayer::~LayersDidChangeSentPerLayer() { }
SdfNotice::LayerInfoDidChange::~LayerInfoDidChange() { }
SdfNotice::LayerIdentifierDidChange::~LayerIdentifierDidChange() { }
SdfNotice::LayerDidReplaceContent::~LayerDidReplaceContent() { }
SdfNotice::LayerDidReloadContent::~LayerDidReloadContent() { }
SdfNotice::LayerDidSaveLayerToFile::~LayerDidSaveLayerToFile() { }
SdfNotice::LayerDirtinessChanged::~LayerDirtinessChanged() { }
SdfNotice::LayerMutenessChanged::~LayerMutenessChanged() { }

PXR_NAMESPACE_CLOSE_SCOPE
