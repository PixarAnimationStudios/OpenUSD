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
#include "pxr/usd/sdf/notice.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/type.h"

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
    layers.reserve(_map->size());
    TF_FOR_ALL(i, *_map) {
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
