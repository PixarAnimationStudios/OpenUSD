//
// Copyright 2023 Pixar
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

#include "hdPrman/rileyTypes.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/rileyRenderOutputSchema.h"
#include "hdPrman/rileyShadingNodeSchema.h"
#include "hdPrman/rileyPrimvarSchema.h"
#include "hdPrman/rileySchemaTypeDefs.h"
#include "hdPrman/rileyParamSchema.h"
#include "hdPrman/rileyPrimvarSchema.h"

#include "hdPrman/rixStrings.h"
#include "hdPrman/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

static
TfToken
_Extract(HdTokenDataSourceHandle const &ds)
{
    if (ds) {
        return ds->GetTypedValue(0.0f);
    } else {
        return {};
    }
}

static
size_t
_Extract(HdSizetDataSourceHandle const &ds)
{
    if (ds) {
        return ds->GetTypedValue(0.0f);
    } else {
        return 0;
    }
}

HdPrman_RileyParamList::HdPrman_RileyParamList(
    HdPrmanRileyParamListSchema schema)
{
    HdPrmanRileyParamContainerSchema containerSchema =
        schema.GetParams();

    for (const TfToken &name : containerSchema.GetNames()) {
        HdPrmanRileyParamSchema paramSchema = containerSchema.Get(name);
        HdSampledDataSourceHandle const ds = paramSchema.GetValue();
        if (!ds) {
            continue;
        }

        // TODO:
        //
        // SetParamFromVtValue is never calling, e.g.,
        // SetFloatReference or SetFloatReferenceArray
        // which is used to set connections between shading nodes by
        // name.
        //
        // We need to designate "role" tokens for these and then
        // add dispatching in SetParamFromVtValue.
        //

        HdPrman_Utils::SetParamFromVtValue(
            RtUString(name.GetText()),
            ds->GetValue(0.0f),
            _Extract(paramSchema.GetRole()),
            &rileyObject);
    }
}

static
RtDetailType
_ToDetailType(const TfToken &t)
{
    if (t == HdPrmanRileyPrimvarSchemaTokens->constant) {
        return RtDetailType::k_constant;
    }

    if (t == HdPrmanRileyPrimvarSchemaTokens->uniform) {
        return RtDetailType::k_uniform;
    }

    if (t == HdPrmanRileyPrimvarSchemaTokens->vertex) {
        return RtDetailType::k_vertex;
    }

    if (t == HdPrmanRileyPrimvarSchemaTokens->facevarying) {
        return RtDetailType::k_facevarying;
    }

    if (t == HdPrmanRileyPrimvarSchemaTokens->varying) {
        return RtDetailType::k_varying;
    }

    if (t == HdPrmanRileyPrimvarSchemaTokens->reference) {
        return RtDetailType::k_reference;
    }

    return RtDetailType::k_invalid;
}

HdPrman_RileyDetailType::HdPrman_RileyDetailType(
    HdTokenDataSourceHandle const &ds)
    : rileyObject(ds
                  ? _ToDetailType(ds->GetTypedValue(0.0f))
                  : RtDetailType::k_constant)
{
}

HdPrman_RileyPrimvarList::HdPrman_RileyPrimvarList(
    HdPrmanRileyPrimvarListSchema schema,
    const GfVec2f &shutterInterval)
  : rileyObject(
      _Extract(schema.GetNumUniform()),
      _Extract(schema.GetNumVertex()),
      _Extract(schema.GetNumVarying()),
      _Extract(schema.GetNumFaceVarying()))
{
    HdPrmanRileyPrimvarContainerSchema containerSchema =
        schema.GetParams();

    if (HdSampledDataSourceHandle const pointsValueDs =
            containerSchema.Get(HdPrmanRileyPrimvarListSchemaTokens->P)
                           .GetValue()) {
        std::vector<HdSampledDataSource::Time> sampleTimes;
        if (pointsValueDs->GetContributingSampleTimesForInterval(
                shutterInterval[0], shutterInterval[1], &sampleTimes)) {
            rileyObject.SetTimes(sampleTimes.size(), sampleTimes.data());
        } else {
            sampleTimes = { 0.0f };
        }

        constexpr RtDetailType detailType = RtDetailType::k_vertex;

        const size_t n = rileyObject.GetNumDetail(detailType);

        for (size_t i = 0; i < sampleTimes.size(); ++i) {
            const VtValue pointsValue = pointsValueDs->GetValue(sampleTimes[i]);
            if (!pointsValue.IsHolding<VtVec3fArray>()) {
                TF_WARN("Primvar 'points' does not contain VtVec3fArray");
                continue;
            }

            const VtVec3fArray pointsArray =
                pointsValue.UncheckedGet<VtVec3fArray>();
            if (pointsArray.size() != n) {
                TF_WARN("Primvar 'points' size (%zu) did not match expected "
                        "expected (%zu)",
                        pointsArray.size(),
                        n);
                continue;
            }

            rileyObject.SetPointDetail(
                RixStr.k_P,
                (RtPoint3*) pointsArray.data(),
                detailType,
                i);
        }
    }

    for (const TfToken &name : containerSchema.GetNames()) {
        if (name == HdPrmanRileyPrimvarListSchemaTokens->P) {
            continue;
        }

        HdPrmanRileyPrimvarSchema primvarSchema = containerSchema.Get(name);
        HdSampledDataSourceHandle const ds = primvarSchema.GetValue();
        if (!ds) {
            continue;
        }

        HdPrman_Utils::SetPrimVarFromVtValue(
            RtUString(name.GetText()),
            ds->GetValue(0.0f),
            HdPrman_RileyDetailType(primvarSchema.GetDetailType()).rileyObject,
            _Extract(primvarSchema.GetRole()),
            &rileyObject);
    }
}

static
riley::ShadingNode::Type
_ToShadingNodeType(const TfToken &t)
{
    if (t == HdPrmanRileyShadingNodeSchemaTokens->pattern) {
        return riley::ShadingNode::Type::k_Pattern;
    }

    if (t == HdPrmanRileyShadingNodeSchemaTokens->bxdf) {
        return riley::ShadingNode::Type::k_Bxdf;
    }

    if (t == HdPrmanRileyShadingNodeSchemaTokens->integrator) {
        return riley::ShadingNode::Type::k_Integrator;
    }

    if (t == HdPrmanRileyShadingNodeSchemaTokens->light) {
        return riley::ShadingNode::Type::k_Light;
    }

    if (t == HdPrmanRileyShadingNodeSchemaTokens->lightFilter) {
        return riley::ShadingNode::Type::k_LightFilter;
    }

    if (t == HdPrmanRileyShadingNodeSchemaTokens->projection) {
        return riley::ShadingNode::Type::k_Projection;
    }

    if (t == HdPrmanRileyShadingNodeSchemaTokens->displacement) {
        return riley::ShadingNode::Type::k_Displacement;
    }

    if (t == HdPrmanRileyShadingNodeSchemaTokens->sampleFilter) {
        return riley::ShadingNode::Type::k_SampleFilter;
    }

    if (t == HdPrmanRileyShadingNodeSchemaTokens->displayFilter) {
        return riley::ShadingNode::Type::k_DisplayFilter;
    }

    return riley::ShadingNode::Type::k_Invalid;
}

HdPrman_RileyShadingNodeType::HdPrman_RileyShadingNodeType(
    HdTokenDataSourceHandle const &ds)
  : rileyObject(ds
                ? _ToShadingNodeType(ds->GetTypedValue(0.0f))
                : riley::ShadingNode::Type::k_Invalid)
{
}

HdPrman_RileyShadingNode::HdPrman_RileyShadingNode(
    HdPrmanRileyShadingNodeSchema schema)
  : rileyObject{
      HdPrman_RileyShadingNodeType(schema.GetType()).rileyObject,
      HdPrman_RileyString(schema.GetName()).rileyObject,
      HdPrman_RileyString(schema.GetHandle()).rileyObject,
      HdPrman_RileyParamList(schema.GetParams()).rileyObject}
{
}

HdPrman_RileyShadingNetwork::HdPrman_RileyShadingNetwork(
    HdPrmanRileyShadingNodeVectorSchema schema)
{
    const size_t n = schema.GetNumElements();
    shadingNodes.reserve(n);

    for (size_t i = 0; i < n; ++i) {
        shadingNodes.push_back(
            HdPrman_RileyShadingNode(schema.GetElement(i))
                .rileyObject);
    }

    rileyObject.nodeCount = n;
    rileyObject.nodes = shadingNodes.data();
}

HdPrman_RileyTransform::HdPrman_RileyTransform(
    HdMatrixDataSourceHandle const &ds,
    const GfVec2f &shutterInterval)
{
    if (!ds) {
        static const RtMatrix4x4 matrix[] = {
            HdPrman_Utils::GfMatrixToRtMatrix(GfMatrix4d(1.0))
        };
        static const float time[] = {
            0.0f
        };
        rileyObject.samples = static_cast<uint32_t>(std::size(matrix));
        rileyObject.matrix = matrix;
        rileyObject.time = time;
        return;
    }

    ds->GetContributingSampleTimesForInterval(
        shutterInterval[0], shutterInterval[1], &time);
    if (time.empty()) {
        time = { 0.0f };
    }

    matrix.reserve(time.size());
    for (const float t : time) {
        matrix.push_back(
            HdPrman_Utils::GfMatrixToRtMatrix(ds->GetTypedValue(t)));
    }

    rileyObject.samples = time.size();
    rileyObject.matrix = matrix.data();
    rileyObject.time = time.data();
}

HdPrman_RileyFloat::HdPrman_RileyFloat(
    HdFloatDataSourceHandle const &ds,
    const float fallbackValue)
  : rileyObject(ds ? ds->GetTypedValue(0.0f) : fallbackValue)
{
}

HdPrman_RileyString::HdPrman_RileyString(
    HdTokenDataSourceHandle const &ds)
{
    if (ds) {
        rileyObject = RtUString(ds->GetTypedValue(0.0f).GetText());
    }
}

HdPrman_RileyExtent::HdPrman_RileyExtent(
    HdVec3iDataSourceHandle const &ds)
  : rileyObject{ 1, 1, 0}
{
    if (ds) {
        const GfVec3i v = ds->GetTypedValue(0.0f);
        rileyObject = { static_cast<uint32_t>(std::max(v[0], 0)),
                        static_cast<uint32_t>(std::max(v[1], 0)),
                        static_cast<uint32_t>(std::max(v[2], 0)) };
    }
}

static
riley::RenderOutputType
_ToRenderOutputType(const TfToken &t)
{
    if (t == HdPrmanRileyRenderOutputSchemaTokens->float_) {
        return riley::RenderOutputType::k_Float;
    }
    if (t == HdPrmanRileyRenderOutputSchemaTokens->integer) {
        return riley::RenderOutputType::k_Integer;
    }
    if (t == HdPrmanRileyRenderOutputSchemaTokens->color) {
        return riley::RenderOutputType::k_Color;
    }
    if (t == HdPrmanRileyRenderOutputSchemaTokens->vector) {
        return riley::RenderOutputType::k_Vector;
    }

    return riley::RenderOutputType::k_Float;
}

HdPrman_RileyRenderOutputType::HdPrman_RileyRenderOutputType(
    HdTokenDataSourceHandle const &ds)
  : rileyObject(ds
                ? _ToRenderOutputType(ds->GetTypedValue(0.0f))
                : riley::RenderOutputType::k_Float)
{
}

HdPrman_RileyFilterSize::HdPrman_RileyFilterSize(
    HdVec2fDataSourceHandle const &ds)
    : rileyObject{1.0f, 1.0f}
{
    if (ds) {
        const GfVec2f v = ds->GetTypedValue(0.0f);
        rileyObject = { v[0], v[1] };
    }
}

HdPrman_RileyUniqueString::HdPrman_RileyUniqueString(
    HdTokenDataSourceHandle const &ds)
{
    std::string name;
    if (ds) {
        name = ds->GetTypedValue(0.0f);
    }

    name += "_";

    static std::atomic_uint_fast64_t id = 0;
    name += std::to_string(id++);

    rileyObject = RtUString(name.c_str());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
