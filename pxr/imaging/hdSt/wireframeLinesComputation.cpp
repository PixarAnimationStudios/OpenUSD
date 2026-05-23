//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/wireframeLinesComputation.h"

#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hdSt/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

static constexpr int _invertEdgeFlag = 1 << 31;

HdSt_WireframeLinesComputation::HdSt_WireframeLinesComputation(bool refined,
    HdBufferSourceSharedPtr const& indices,
    HdBufferSourceSharedPtrVector const& fvarIndices)
    : _indices(indices)
    , _fvarIndices(fvarIndices)
    , _refined(refined)
{
}

void
HdSt_WireframeLinesComputation::GetBufferSpecs(HdBufferSpecVector* specs) const
{
    specs->emplace_back(HdTokens->indices, HdTupleType{HdTypeInt32Vec2, 1});
    specs->emplace_back(HdTokens->edgeIndices, HdTupleType{HdTypeInt32, 1});
    specs->emplace_back(HdTokens->primitiveParam,
        HdTupleType{_refined ? HdTypeInt32Vec4 : HdTypeInt32Vec3, 1});
    for (const auto& indices : _fvarIndices) {
        indices->GetBufferSpecs(specs);
    }
}

bool
HdSt_WireframeLinesComputation::Resolve()
{
    if (!TF_VERIFY(_indices)) {
        return false;
    }

    if (!_indices->IsResolved()) {
        return false;
    }

    for (const auto& chainedBuffer : _indices->GetChainedBuffers()) {
        if (chainedBuffer->GetName() == HdTokens->edgeIndices) {
            _edgeIndices = chainedBuffer;
        } else if (chainedBuffer->GetName() == HdTokens->primitiveParam) {
            _primitiveParam = chainedBuffer;
        }
    }

    if (!TF_VERIFY(_edgeIndices) || !TF_VERIFY(_primitiveParam)) {
        return false;
    }

    if (!_edgeIndices->IsResolved() || !_primitiveParam->IsResolved()) {
        return false;
    }

    for (const auto& indices : _fvarIndices) {
        if (!indices->IsResolved()) {
            return false;
        }
    }

    if (!_TryLock()) {
        return false;
    }

    if (!_indices->GetNumElements()) {
        _SetResolved();
        return true;
    }

    bool success = false;
    const auto& tuple = _indices->GetTupleType();
    if (tuple.type == HdTypeInt32Vec3) {
        _ResolveTris();
        success = true;
    } else if (tuple.type == HdTypeInt32 && tuple.count == 6) {
        _ResolveQuads();
        success = true;
    }

    if (success) {
        _SetResolved();
    } else {
        _SetResolveError();
        TF_CODING_ERROR("HdSt_WireframeLinesComputation only supports "
                        "triangles and triangulated quads.");
    }

    return true;
}

void
HdSt_WireframeLinesComputation::_ResolveTris()
{
    HD_TRACE_FUNCTION();

    const auto triIndices = static_cast<const GfVec3i*>(_indices->GetData());

    const int* coarseTriEdgeIndices = nullptr;
    const GfVec2i* refinedTriEdgeIndices = nullptr;
    const int* coarsePrimitiveParam = nullptr;
    const GfVec3i* refinedPrimitiveParam = nullptr;
    if (_refined) {
        refinedTriEdgeIndices =
            static_cast<const GfVec2i*>(_edgeIndices->GetData());
        refinedPrimitiveParam =
            static_cast<const GfVec3i*>(_primitiveParam->GetData());
    } else {
        coarseTriEdgeIndices = static_cast<const int*>(_edgeIndices->GetData());
        coarsePrimitiveParam =
            static_cast<const int*>(_primitiveParam->GetData());
    }

    const int triCount = static_cast<int>(_indices->GetNumElements());
    const size_t sizeGuess = triCount * 3 / 2;
    VtVec2iArray lineIndices;
    lineIndices.reserve(sizeGuess);
    VtIntArray lineEdgeIndices;
    lineEdgeIndices.reserve(sizeGuess);
    VtVec3iArray coarseLinePrimitiveParam;
    VtVec4iArray refinedLinePrimitiveParam;
    if (_refined) {
        refinedLinePrimitiveParam.reserve(sizeGuess);
    } else {
        coarseLinePrimitiveParam.reserve(sizeGuess);
    }
    std::vector<VtVec3iArray> lineFvarIndices;
    for (size_t i = 0; i < _fvarIndices.size(); i++) {
        lineFvarIndices.emplace_back().reserve(sizeGuess);
    }

    for (int tri = 0; tri < triCount; ++tri) {
        const int i0 = triIndices[tri][0];
        const int i1 = triIndices[tri][1];
        const int i2 = triIndices[tri][2];

        const int triEdgeIndex = _refined ? refinedTriEdgeIndices[tri][0] :
                                            coarseTriEdgeIndices[tri];

        const int faceIndex = HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam(
            _refined ? refinedPrimitiveParam[tri][0] :
                       coarsePrimitiveParam[tri]);
        const int edgeFlag = HdMeshUtil::DecodeEdgeFlagFromCoarseFaceParam(
            _refined ? refinedPrimitiveParam[tri][0] :
                       coarsePrimitiveParam[tri]);
        const int thirdParam = _refined ? refinedPrimitiveParam[tri][2] : 0;

        const auto copyPrimitiveParams = [&](int localEdgeId,
                                             int facingParameter) {
            if (_refined) {
                refinedLinePrimitiveParam.push_back(
                    {HdMeshUtil::EncodeCoarseFaceParam(faceIndex, localEdgeId),
                        tri, thirdParam, facingParameter});
            } else {
                coarseLinePrimitiveParam.push_back(
                    {HdMeshUtil::EncodeCoarseFaceParam(faceIndex, localEdgeId),
                        tri, facingParameter});
            }
        };
        const auto copyFvarIndices = [&]() {
            for (size_t f = 0; f < _fvarIndices.size(); f++) {
                const auto fvar = static_cast<const GfVec3i*>(
                    _fvarIndices[f]->GetData())[tri];
                lineFvarIndices[f].push_back(fvar);
            }
        };

        // Based on EdgeId.Fragment.TriangleParam GetPrimitiveEdgeId()
        const int edgeIndexOffset = edgeFlag == 0 ? 0 : -1;
        if (!(edgeFlag & 2)) {
            lineIndices.push_back({i1, i0});
            lineEdgeIndices.push_back(triEdgeIndex + 0 + edgeIndexOffset);
            copyPrimitiveParams(0, i2 | _invertEdgeFlag);
            copyFvarIndices();
        }

        lineIndices.push_back({i1, i2});
        lineEdgeIndices.push_back(triEdgeIndex + 1 + edgeIndexOffset);
        copyPrimitiveParams(1, i0);
        copyFvarIndices();

        if (!(edgeFlag & 1)) {
            lineIndices.push_back({i2, i0});
            lineEdgeIndices.push_back(triEdgeIndex + 2 + edgeIndexOffset);
            copyPrimitiveParams(2, i1);
            copyFvarIndices();
        }
    }

    _SetResult(std::make_shared<HdVtBufferSource>(
        HdTokens->indices, VtValue(std::move(lineIndices))));
    _lineEdgeIndices = std::make_shared<HdVtBufferSource>(
        HdTokens->edgeIndices, VtValue(std::move(lineEdgeIndices)));
    _linePrimitiveParam =
        std::make_shared<HdVtBufferSource>(HdTokens->primitiveParam,
            _refined ? VtValue(std::move(refinedLinePrimitiveParam)) :
                       VtValue(std::move(coarseLinePrimitiveParam)));
    for (size_t f = 0; f < _fvarIndices.size(); f++) {
        _lineFvarIndices.push_back(
            std::make_shared<HdVtBufferSource>(_fvarIndices[f]->GetName(),
                VtValue(std::move(lineFvarIndices[f]))));
    }
}

void
HdSt_WireframeLinesComputation::_ResolveQuads()
{
    HD_TRACE_FUNCTION();

    const auto quadIndices = static_cast<const int*>(_indices->GetData());
    const auto quadEdgeIndices =
        static_cast<const GfVec2i*>(_edgeIndices->GetData());

    const int* coarsePrimitiveParam = nullptr;
    const GfVec3i* refinedPrimitiveParam = nullptr;
    if (_refined) {
        refinedPrimitiveParam =
            static_cast<const GfVec3i*>(_primitiveParam->GetData());
    } else {
        coarsePrimitiveParam =
            static_cast<const int*>(_primitiveParam->GetData());
    }

    const int quadCount = static_cast<int>(_indices->GetNumElements());
    const size_t sizeGuess = quadCount * 2;
    VtVec2iArray lineIndices;
    lineIndices.reserve(sizeGuess);
    VtIntArray lineEdgeIndices;
    lineEdgeIndices.reserve(sizeGuess);
    VtVec3iArray coarseLinePrimitiveParam;
    VtVec4iArray refinedLinePrimitiveParam;
    if (_refined) {
        refinedLinePrimitiveParam.reserve(sizeGuess);
    } else {
        coarseLinePrimitiveParam.reserve(sizeGuess);
    }
    std::vector<VtVec4iArray> lineFvarIndices;
    for (size_t i = 0; i < _fvarIndices.size(); i++) {
        lineFvarIndices.emplace_back().reserve(sizeGuess);
    }

    for (int quad = 0; quad < quadCount; ++quad) {
        const int base = quad * 6; // triquads
        const int i0 = quadIndices[base + 0];
        const int i1 = quadIndices[base + 1];
        const int i2 = quadIndices[base + 2];
        const int i3 = quadIndices[base + 4];

        const int faceIndex = HdMeshUtil::DecodeFaceIndexFromCoarseFaceParam(
            _refined ? refinedPrimitiveParam[quad][0] :
                       coarsePrimitiveParam[quad]);
        const int edgeFlag = HdMeshUtil::DecodeEdgeFlagFromCoarseFaceParam(
            _refined ? refinedPrimitiveParam[quad][0] :
                       coarsePrimitiveParam[quad]);
        const int thirdParam = _refined ? refinedPrimitiveParam[quad][2] : 0;

        const auto copyPrimitiveParams = [&](int localEdgeId,
                                             int facingParameter) {
            if (_refined) {
                refinedLinePrimitiveParam.push_back(
                    {HdMeshUtil::EncodeCoarseFaceParam(faceIndex, localEdgeId),
                        quad, thirdParam, facingParameter});
            } else {
                coarseLinePrimitiveParam.push_back(
                    {HdMeshUtil::EncodeCoarseFaceParam(faceIndex, localEdgeId),
                        quad, facingParameter});
            }
        };
        const auto copyFvarIndices = [&]() {
            for (size_t f = 0; f < _fvarIndices.size(); f++) {
                const auto fvar = static_cast<const GfVec4i*>(
                    _fvarIndices[f]->GetData())[quad];
                lineFvarIndices[f].push_back(fvar);
            }
        };

        // Based on EdgeId.Fragment.QuadParam GetPrimitiveEdgeId()
        lineIndices.push_back({i1, i0});
        lineEdgeIndices.push_back(
            edgeFlag ? quadEdgeIndices[quad][0] : quadEdgeIndices[quad][0] + 0);
        copyPrimitiveParams(0, i2 | _invertEdgeFlag);
        copyFvarIndices();

        if (_refined || !edgeFlag) {
            lineIndices.push_back({i1, i2});
            lineEdgeIndices.push_back(quadEdgeIndices[quad][0] + 1);
            copyPrimitiveParams(1, i0);
            copyFvarIndices();
        }

        if (_refined || !edgeFlag) {
            lineIndices.push_back({i3, i2});
            lineEdgeIndices.push_back(quadEdgeIndices[quad][0] + 2);
            copyPrimitiveParams(2, i1 | _invertEdgeFlag);
            copyFvarIndices();
        }

        lineIndices.push_back({i3, i0});
        lineEdgeIndices.push_back(
            edgeFlag ? quadEdgeIndices[quad][1] : quadEdgeIndices[quad][0] + 3);
        copyPrimitiveParams(3, i2);
        copyFvarIndices();
    }

    _SetResult(std::make_shared<HdVtBufferSource>(
        HdTokens->indices, VtValue(std::move(lineIndices))));
    _lineEdgeIndices = std::make_shared<HdVtBufferSource>(
        HdTokens->edgeIndices, VtValue(std::move(lineEdgeIndices)));
    _linePrimitiveParam =
        std::make_shared<HdVtBufferSource>(HdTokens->primitiveParam,
            _refined ? VtValue(std::move(refinedLinePrimitiveParam)) :
                       VtValue(std::move(coarseLinePrimitiveParam)));
    for (size_t f = 0; f < _fvarIndices.size(); f++) {
        _lineFvarIndices.push_back(
            std::make_shared<HdVtBufferSource>(_fvarIndices[f]->GetName(),
                VtValue(std::move(lineFvarIndices[f]))));
    }
}

bool
HdSt_WireframeLinesComputation::HasPreChainedBuffer() const
{
    return true;
}

HdBufferSourceSharedPtrVector
HdSt_WireframeLinesComputation::GetPreChainedBuffers() const
{
    HdBufferSourceSharedPtrVector preChainedBuffers{_indices};
    preChainedBuffers.insert(
        preChainedBuffers.end(), _fvarIndices.begin(), _fvarIndices.end());
    return preChainedBuffers;
}

bool
HdSt_WireframeLinesComputation::HasChainedBuffer() const
{
    return true;
}

HdBufferSourceSharedPtrVector
HdSt_WireframeLinesComputation::GetChainedBuffers() const
{
    HdBufferSourceSharedPtrVector chainedBuffers;
    if (_lineEdgeIndices) {
        chainedBuffers.push_back(_lineEdgeIndices);
    }
    if (_linePrimitiveParam) {
        chainedBuffers.push_back(_linePrimitiveParam);
    }
    chainedBuffers.insert(
        chainedBuffers.end(), _lineFvarIndices.begin(), _lineFvarIndices.end());
    return chainedBuffers;
}

bool
HdSt_WireframeLinesComputation::_CheckValid() const
{
    for (const auto& indices : _fvarIndices) {
        if (!indices->IsValid()) {
            return false;
        }
    }
    return _indices != nullptr && _indices->IsValid();
}

PXR_NAMESPACE_CLOSE_SCOPE
