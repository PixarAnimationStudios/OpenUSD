//
// Copyright 2020 Pixar
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


#include "pxr/imaging/pxOsd/meshTopologyValidation.h"
#include "pxr/imaging/pxOsd/meshTopology.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

#include <algorithm>
#include <numeric>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum) {
    TF_ADD_ENUM_NAME(PxOsdMeshTopologyValidation::Code::InvalidScheme);
    TF_ADD_ENUM_NAME(PxOsdMeshTopologyValidation::Code::InvalidOrientation);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidTriangleSubdivision);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidVertexInterpolationRule);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidFaceVaryingInterpolationRule);
    TF_ADD_ENUM_NAME(PxOsdMeshTopologyValidation::Code::InvalidCreaseMethod);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidCreaseLengthElement);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidCreaseIndicesSize);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidCreaseIndicesElement);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidCreaseWeightsSize);
    TF_ADD_ENUM_NAME(PxOsdMeshTopologyValidation::Code::NegativeCreaseWeights);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidCornerIndicesElement);
    TF_ADD_ENUM_NAME(PxOsdMeshTopologyValidation::Code::NegativeCornerWeights);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidCornerWeightsSize);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidFaceVertexCountsElement);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidFaceVertexIndicesElement);
    TF_ADD_ENUM_NAME(
        PxOsdMeshTopologyValidation::Code::InvalidFaceVertexIndicesSize);
};

PxOsdMeshTopologyValidation::PxOsdMeshTopologyValidation(
    PxOsdMeshTopology const& topology)
{
    TRACE_FUNCTION();
    _ValidateScheme(topology);
    _ValidateOrientation(topology);
    _ValidateTriangleSubdivision(topology);
    _ValidateVertexInterpolation(topology);
    _ValidateFaceVaryingInterpolation(topology);
    _ValidateCreaseMethod(topology);
    _ValidateCreasesAndCorners(topology);
    _ValidateHoles(topology);
    _ValidateFaceVertexCounts(topology);
    _ValidateFaceVertexIndices(topology);
}

template <size_t S>
void
PxOsdMeshTopologyValidation::_ValidateToken(
    PxOsdMeshTopologyValidation::Code code, const char* name,
    const TfToken& token, const std::array<TfToken, S>& validTokens)
{
    if (!std::any_of(std::cbegin(validTokens), std::cend(validTokens),
                     [&token](const TfToken& validToken) {
                         return token == validToken;
                     })) {
        _AppendInvalidation(
            {code, TfStringPrintf("'%s' is not a valid '%s' token.",
                                  token.GetText(), name)});
    }
}

void
PxOsdMeshTopologyValidation::_ValidateScheme(
    PxOsdMeshTopology const& topology)
{
    static const std::array<TfToken, 3> validSchemes = {
        {PxOsdOpenSubdivTokens->catmullClark, PxOsdOpenSubdivTokens->loop,
         PxOsdOpenSubdivTokens->bilinear}};
    _ValidateToken(Code::InvalidScheme, "scheme", topology.GetScheme(),
                   validSchemes);
}

void
PxOsdMeshTopologyValidation::_ValidateOrientation(
    PxOsdMeshTopology const& topology)
{
    static const std::array<TfToken, 2> validOrientations = {
        {PxOsdOpenSubdivTokens->rightHanded,
         PxOsdOpenSubdivTokens->leftHanded}};
    _ValidateToken(Code::InvalidOrientation, "orientation",
                   topology.GetOrientation(), validOrientations);
}

void
PxOsdMeshTopologyValidation::_ValidateTriangleSubdivision(
    PxOsdMeshTopology const& topology)
{
    static const TfToken empty;
    static const std::array<TfToken, 3> validTriangleSubdivision = {
        {PxOsdOpenSubdivTokens->catmullClark, PxOsdOpenSubdivTokens->smooth,
         empty}};
    _ValidateToken(Code::InvalidTriangleSubdivision, "triangle subdivision",
                   topology.GetSubdivTags().GetTriangleSubdivision(),
                   validTriangleSubdivision);
}

void
PxOsdMeshTopologyValidation::_ValidateVertexInterpolation(
    PxOsdMeshTopology const& topology)
{
    static const TfToken empty;
    static const std::array<TfToken, 4> validVertexInterpolation = {
        {PxOsdOpenSubdivTokens->none, PxOsdOpenSubdivTokens->edgeAndCorner,
         PxOsdOpenSubdivTokens->edgeOnly, empty}};
    _ValidateToken(Code::InvalidVertexInterpolationRule,
                   "vertex interpolation rule",
                   topology.GetSubdivTags().GetVertexInterpolationRule(),
                   validVertexInterpolation);
}

void
PxOsdMeshTopologyValidation::_ValidateFaceVaryingInterpolation(
    PxOsdMeshTopology const& topology)
{
    static const TfToken empty;
    static const std::array<TfToken, 7> validFaceVaryingInterpolation = {
        {PxOsdOpenSubdivTokens->none, PxOsdOpenSubdivTokens->all,
         PxOsdOpenSubdivTokens->boundaries, PxOsdOpenSubdivTokens->cornersOnly,
         PxOsdOpenSubdivTokens->cornersPlus1,
         PxOsdOpenSubdivTokens->cornersPlus2, empty}};
    _ValidateToken(Code::InvalidFaceVaryingInterpolationRule,
                   "face varying interpolation rule",
                   topology.GetSubdivTags().GetFaceVaryingInterpolationRule(),
                   validFaceVaryingInterpolation);
}

void
PxOsdMeshTopologyValidation::_ValidateCreaseMethod(
    PxOsdMeshTopology const& topology)
{
    static const TfToken empty;
    static const std::array<TfToken, 3> validCreaseMethod = {
        {PxOsdOpenSubdivTokens->uniform, PxOsdOpenSubdivTokens->chaikin,
         empty}};
    _ValidateToken(Code::InvalidCreaseMethod, "crease method",
                   topology.GetSubdivTags().GetCreaseMethod(),
                   validCreaseMethod);
}

void
PxOsdMeshTopologyValidation::_ValidateCreasesAndCorners(
    PxOsdMeshTopology const& topology)
{
    const auto& creaseIndices = topology.GetSubdivTags().GetCreaseIndices();
    const auto& creaseLengths = topology.GetSubdivTags().GetCreaseLengths();
    const auto& creaseSharpness = topology.GetSubdivTags().GetCreaseWeights();
    const auto& cornerIndices = topology.GetSubdivTags().GetCornerIndices();
    const auto& cornerSharpness = topology.GetSubdivTags().GetCornerWeights();

    if (std::any_of(creaseLengths.cbegin(), creaseLengths.cend(),
                    [](int creaseLength) { return creaseLength < 2; })) {
        _AppendInvalidation(
            {Code::InvalidCreaseLengthElement,
             "Crease lengths must be greater than or equal to 2."});
    }
    size_t totalCreaseIndices =
        std::accumulate(creaseLengths.cbegin(), creaseLengths.cend(), 0);
    size_t totalCreases = creaseLengths.size();
    size_t totalCreaseEdges = totalCreaseIndices - totalCreases;
    if (creaseIndices.size() != totalCreaseIndices) {
        _AppendInvalidation(
            {Code::InvalidCreaseIndicesSize,
             TfStringPrintf(
                 "Crease indices size '%zu' doesn't match expected '%zu'.",
                 creaseIndices.size(), totalCreaseIndices)});
    }
    if (creaseSharpness.size() != totalCreaseEdges &&
        creaseSharpness.size() != totalCreases) {
        _AppendInvalidation(
            {Code::InvalidCreaseWeightsSize,
             TfStringPrintf(
                 "Crease weights size '%zu' doesn't match either per edge "
                 "'%zu' or per crease '%zu' sizes.",
                 creaseSharpness.size(), totalCreaseEdges,
                 totalCreases)});
    }

    if (cornerIndices.size() != cornerSharpness.size()) {
        _AppendInvalidation(
            {Code::InvalidCornerWeightsSize,
             TfStringPrintf(
                 "Corner weights size '%zu' doesn't match expected '%zu'.",
                 cornerIndices.size(), cornerSharpness.size())});
    }

    if (std::any_of(creaseSharpness.cbegin(), creaseSharpness.cend(),
                    [](float weight) { return weight < 0.0f; })) {
        _AppendInvalidation(
            {Code::NegativeCreaseWeights, "Negative crease weights."});
    }

    if (std::any_of(cornerSharpness.cbegin(), cornerSharpness.cend(),
                    [](float weight) { return weight < 0.0f; })) {
        _AppendInvalidation(
            {Code::NegativeCornerWeights, "Negative corner weights."});
    }

    std::vector<int> sortedFaceIndices;
    sortedFaceIndices.assign(topology.GetFaceVertexIndices().cbegin(),
                             topology.GetFaceVertexIndices().cend());
    std::sort(std::begin(sortedFaceIndices), std::end(sortedFaceIndices));

    if (std::any_of(cornerIndices.cbegin(), cornerIndices.cend(),
                    [&sortedFaceIndices](int index) {
                        return std::find(sortedFaceIndices.begin(),
                                         sortedFaceIndices.end(),
                                         index) == sortedFaceIndices.end();
                    })) {
        _AppendInvalidation(
            {Code::InvalidCornerIndicesElement,
             "Corner index element missing from face vertex indices array."});
    }
    if (std::any_of(creaseIndices.cbegin(), creaseIndices.cend(),
                    [&sortedFaceIndices](int index) {
                        return std::find(sortedFaceIndices.begin(),
                                         sortedFaceIndices.end(),
                                         index) == sortedFaceIndices.end();
                    })) {
        _AppendInvalidation(
            {Code::InvalidCreaseIndicesElement,
             "Crease index element missing from face vertex indices array."});
    }
}

void
PxOsdMeshTopologyValidation::_ValidateHoles(
    PxOsdMeshTopology const& topology)
{
    const auto& holeIndices = topology.GetHoleIndices();
    if (holeIndices.empty()) return;
    const auto holeIndexRange =
        std::minmax_element(holeIndices.cbegin(), holeIndices.cend());
    if (*holeIndexRange.first < 0) {
        _AppendInvalidation({Code::InvalidHoleIndicesElement,
                             "Hole indices cannot be negative."});
    }
    if (*holeIndexRange.second >= (int)topology.GetFaceVertexCounts().size()) {
        _AppendInvalidation(
            {Code::InvalidHoleIndicesElement,
             TfStringPrintf("Hole indices must be less than face count '%zu'.",
                            topology.GetFaceVertexCounts().size())});
    }
}

void
PxOsdMeshTopologyValidation::_ValidateFaceVertexCounts(
    PxOsdMeshTopology const& topology)
{
    const auto& faceVertexCounts = topology.GetFaceVertexCounts();
    if (std::any_of(faceVertexCounts.cbegin(), faceVertexCounts.cend(),
                    [](int faceVertexCount) { return faceVertexCount <= 2; })) {
        _AppendInvalidation({Code::InvalidFaceVertexCountsElement,
                             "Face vertex counts must be greater than 2."});
    }
}

void
PxOsdMeshTopologyValidation::_ValidateFaceVertexIndices(
    PxOsdMeshTopology const& topology)
{
    if (std::any_of(topology.GetFaceVertexIndices().cbegin(),
                    topology.GetFaceVertexIndices().cend(),
                    [](int i) { return i < 0; })) {
        _AppendInvalidation(
            {Code::InvalidFaceVertexIndicesElement,
             "Face vertex indices element must be greater than 0."});
    }
    const auto& faceVertexCounts = topology.GetFaceVertexCounts();
    size_t vertexCount =
        std::accumulate(faceVertexCounts.cbegin(), faceVertexCounts.cend(), 0);
    if (topology.GetFaceVertexIndices().size() != vertexCount) {
        _AppendInvalidation(
            {Code::InvalidFaceVertexIndicesSize,
             TfStringPrintf("Face vertex indices size '%zu' does not match "
                            "expected size '%zu'.",
                            topology.GetFaceVertexIndices().size(),
                            vertexCount)});
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
