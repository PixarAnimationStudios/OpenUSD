//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_SMOOTH_NORMALS_H
#define PXR_IMAGING_HD_ST_SMOOTH_NORMALS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/computation.h"

#include "pxr/imaging/hd/bufferSource.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


class Hd_VertexAdjacency;
class HdSt_VertexAdjacencyBuilder;

/// \class HdSt_SmoothNormalsComputationCPU
///
/// Smooth normal computation CPU.
///
class HdSt_SmoothNormalsComputationCPU : public HdComputedBufferSource
{
public:
    HDST_API
    HdSt_SmoothNormalsComputationCPU(
        Hd_VertexAdjacency const *adjacency,
        HdBufferSourceSharedPtr const &points,
        TfToken const &dstName,
        HdBufferSourceSharedPtr const &adjacencyBuilder,
        bool packed);

    HDST_API
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;

    HDST_API
    bool Resolve() override;

    HDST_API
    TfToken const &GetName() const override;

protected:
    HDST_API
    bool _CheckValid() const override;

private:
    Hd_VertexAdjacency const *_adjacency;
    HdBufferSourceSharedPtr _points;
    TfToken _dstName;
    HdBufferSourceSharedPtr _adjacencyBuilder;
    bool _packed;
};

/// \class HdSt_SmoothNormalsComputationGPU
///
/// Smooth normal computation GPU.
///
class HdSt_SmoothNormalsComputationGPU : public HdStComputation
{
public:
    HDST_API
    HdSt_SmoothNormalsComputationGPU(
        HdSt_VertexAdjacencyBuilder const *vertexAdjacencyBuilder,
        TfToken const &srcName,
        TfToken const &dstName,
        HdType srcDataType,
        bool packed);

    HDST_API
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;

    HDST_API
    void Execute(HdBufferArrayRangeSharedPtr const &range,
                 HdResourceRegistry *resourceRegistry) override;

    /// This computation doesn't generate buffer source (i.e. 2nd phase)
    /// This is a gpu computation, but no need to resize the destination
    /// since it belongs the same range as src buffer.
    int GetNumOutputElements() const override { return 0; }

private:
    HdSt_VertexAdjacencyBuilder const *_vertexAdjacencyBuilder;
    TfToken _srcName;
    TfToken _dstName;
    HdType _srcDataType;
    HdType _dstDataType;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_SMOOTH_NORMALS_H
