//
// Copyright 2017 Pixar
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
#include "pxrUsdInShipped/pointInstancerUtils.h"

#include "pxr/pxr.h"
#include "pxr/base/gf/transform.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX This is based on UsdGeomPointInstancer::ComputeInstanceTransformsAtTime.
// Ideally, we would just use UsdGeomPointInstancer, but it does not currently
// support multi-sampled transforms.
//
void
PxrUsdInShipped_PointInstancerUtils::ComputeInstanceTransformsAtTime(
        std::vector<std::vector<GfMatrix4d>> &xforms,
        size_t &numXformSamples,
        const UsdGeomPointInstancer &instancer,
        const std::vector<UsdTimeCode> &sampleTimes,
        const UsdTimeCode baseTime)
{
    const auto sampleCount = sampleTimes.size();
    if (sampleCount == 0 || xforms.size() < sampleCount ||
        baseTime.IsDefault()) {
        return;
    }

    UsdAttribute positionsAttr = instancer.GetPositionsAttr();
    if (!positionsAttr.HasValue()) {
        return;
    }

    bool positionsHasSamples = false;
    double positionsLowerTimeSample = 0.0;
    double upperTimeSample = 0.0;
    if (!positionsAttr.GetBracketingTimeSamples(
            baseTime.GetValue(), &positionsLowerTimeSample,
            &upperTimeSample, &positionsHasSamples)) {
        return;
    }

    VtIntArray protoIndices;
    if (!instancer.GetProtoIndicesAttr().Get(&protoIndices, baseTime)) {
        return;
    }

    const auto numInstances = protoIndices.size();

    const auto scalesAttr = instancer.GetScalesAttr();
    const auto orientationsAttr = instancer.GetOrientationsAttr();

    VtVec3fArray positions;
    VtVec3fArray scales;
    VtQuathArray orientations;

    for (auto a = decltype(sampleCount){0}; a < sampleCount; ++a) {
        std::vector<GfMatrix4d> &curr = xforms[a];
        curr.reserve(numInstances);

        // Get sample-dependent values. Stop if topology differs, but permit
        // unspecified scales and orientations.
        positionsAttr.Get(&positions, sampleTimes[a]);
        if (positions.size() != numInstances) {
            break;
        }
        scalesAttr.Get(&scales, sampleTimes[a]);
        orientationsAttr.Get(&orientations, sampleTimes[a]);
        if (scales.size() > 0 and scales.size() != numInstances) {
            break;
        }
        if (orientations.size() > 0 and orientations.size() != numInstances) {
            break;
        }

        for (auto i = decltype(numInstances){0}; i < numInstances; ++i) {
            GfTransform transform;
            transform.SetTranslation(positions[i]);
            if (scales.size() > 0) {
                transform.SetScale(scales[i]);
            }
            if (orientations.size() > 0) {
                transform.SetRotation(GfRotation(orientations[i]));
            }
            curr.push_back(transform.GetMatrix());
        }

        ++numXformSamples;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
