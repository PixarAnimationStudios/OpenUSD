//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#ifndef HD_USD_WRITER_LIGHT_H
#define HD_USD_WRITER_LIGHT_H

#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/js/json.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/pxr.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdUsdWriterLight
///
class HdUsdWriterLight : public HdLight
{
public:
    HF_MALLOC_TAG_NEW("new HdUsdWriterLight");

    /// HdUsdWriterLight constructor.
    ///   \param id The scene-graph path to this light.
    HDUSDWRITER_API
    HdUsdWriterLight(TfToken const& type, SdfPath const& id);

    /// HdUsdWriterLight destructor.
    HDUSDWRITER_API
    ~HdUsdWriterLight() override = default;

    /// Inform the scene graph which state needs to be downloaded in the
    /// first Sync() call: in this case, topology and Light data to build
    /// the geometry object in the scene graph.
    ///   \return The initial dirty state this light wants to query.
    HDUSDWRITER_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Pull invalidated scene data and prepare/update the renderable
    /// representation.
    ///   \param sceneDelegate The data source for this geometry item.
    ///   \param renderParam State.
    ///   \param dirtyBits A specifier for which scene data has changed.
    ///
    HDUSDWRITER_API
    void Sync(HdSceneDelegate* sceneDelegate, HdRenderParam* renderParam, HdDirtyBits* dirtyBits) override;

    /// Serialize the primitive to USD.
    ///
    ///   \param stage Reference to HdUsdWriter Stage Proxy.
    HDUSDWRITER_API
    virtual void SerializeToUsd(const UsdStagePtr &stage);

protected:
    // This class does not support copying.
    HdUsdWriterLight(const HdUsdWriterLight&) = delete;
    HdUsdWriterLight& operator=(const HdUsdWriterLight&) = delete;

    HdUsdWriterOptional<GfMatrix4d> _transform;
    HdUsdWriterOptional<SdfPath> _materialId;
    HdUsdWriterOptional<bool> _visible;
    std::unordered_map<TfToken, VtValue, TfToken::HashFunctor> _params;
    const TfToken _type;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif