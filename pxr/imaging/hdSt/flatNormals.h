//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_FLAT_NORMALS_H
#define PXR_IMAGING_HD_ST_FLAT_NORMALS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/computation.h"

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/flatNormals.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdSt_FlatNormalsComputationCPU
///
/// Flat normal computation CPU.
///
class HdSt_FlatNormalsComputationCPU : public HdComputedBufferSource
{
public:
    HDST_API
    HdSt_FlatNormalsComputationCPU(
        HdMeshTopology const *topology,
        HdBufferSourceSharedPtr const &points,
        TfToken const &dstName,
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
    HdMeshTopology const *_topology;
    HdBufferSourceSharedPtr const _points;
    TfToken _dstName;
    bool _packed;
};

/// \class HdSt_FlatNormalsComputationGPU
///
/// Flat normal computation GPU.
///
class HdSt_FlatNormalsComputationGPU : public HdStComputation
{
public:
    HDST_API
    HdSt_FlatNormalsComputationGPU(
        HdBufferArrayRangeSharedPtr const &topologyRange,
        HdBufferArrayRangeSharedPtr const &vertexRange,
        int numFaces,
        TfToken const &srcName,
        TfToken const &dstName,
        HdType srcDataType,
        bool packed);

    HDST_API
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;

    HDST_API
    void Execute(HdBufferArrayRangeSharedPtr const &range,
                 HdResourceRegistry *resourceRegistry) override;

    int GetNumOutputElements() const override;

private:
    HdBufferArrayRangeSharedPtr const _topologyRange;
    HdBufferArrayRangeSharedPtr const _vertexRange;
    int _numFaces;
    TfToken _srcName;
    TfToken _dstName;
    HdType _srcDataType;
    HdType _dstDataType;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_FLAT_NORMALS_H
