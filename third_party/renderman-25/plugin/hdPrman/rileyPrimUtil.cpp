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

#include "hdPrman/rileyPrimUtil.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "hdPrman/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
