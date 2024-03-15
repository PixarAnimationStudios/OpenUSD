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

#ifndef HD_USD_WRITER__BASIC_CURVES_H
#define HD_USD_WRITER__BASIC_CURVES_H

#include "pxr/usdImaging/plugin/hdUsdWriter/pointBased.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/api.h"
#include "pxr/pxr.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/basisCurvesTopology.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdUsdWriterBasisCurves
///
class HdUsdWriterBasisCurves : public HdUsdWriterPointBased<HdBasisCurves>
{
public:
    HF_MALLOC_TAG_NEW("new HdUsdWriterBasisCurves");

    /// HdUsdWriterBasisCurves constructor.
    ///   \param id The scene-graph path to this curves.
    HDUSDWRITER_API
    explicit HdUsdWriterBasisCurves(SdfPath const& id);

    /// HdUsdWriterBasisCurves destructor.
    HDUSDWRITER_API
    ~HdUsdWriterBasisCurves() override = default;

    /// Inform the scene graph which state needs to be downloaded in the
    /// first Sync() call: in this case, topology and points data to build
    /// the geometry object in the scene graph.
    ///   \return The initial dirty state this curves wants to query.
    HDUSDWRITER_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Pull invalidated scene data and prepare/update the renderable
    /// representation.
    ///   \param sceneDelegate The data source for this geometry item.
    ///   \param renderParam State.
    ///   \param dirtyBits A specifier for which scene data has changed.
    ///   \param reprToken A specifier for which representation to draw with.
    ///
    HDUSDWRITER_API
    void Sync(HdSceneDelegate* sceneDelegate,
              HdRenderParam* renderParam,
              HdDirtyBits* dirtyBits,
              TfToken const& reprToken) override;

    /// Serialize the primitive to USD.
    ///
    ///   \param stage Reference to HdUsdWriter Stage Proxy.
    HDUSDWRITER_API
    virtual void SerializeToUsd(const UsdStagePtr &stage);

protected:
    // This class does not support copying.
    HdUsdWriterBasisCurves(const HdUsdWriterBasisCurves&) = delete;
    HdUsdWriterBasisCurves& operator=(const HdUsdWriterBasisCurves&) = delete;

    HdUsdWriterOptional<HdBasisCurvesTopology> _topology;
    HdUsdWriterOptional<HdDisplayStyle> _displayStyle;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif