//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HD_ST_WIREFRAME_LINES_COMPUTATION_H
#define PXR_IMAGING_HD_ST_WIREFRAME_LINES_COMPUTATION_H

#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdSt_WireframeLinesComputation
///
class HdSt_WireframeLinesComputation : public HdComputedBufferSource
{
public:
    HDST_API
    HdSt_WireframeLinesComputation(bool refined,
        HdBufferSourceSharedPtr const& triIndices,
        HdBufferSourceSharedPtrVector const& fvarIndices = {});

    HDST_API
    void GetBufferSpecs(HdBufferSpecVector* specs) const override;

    HDST_API
    bool Resolve() override;

    HDST_API
    bool HasPreChainedBuffer() const override;

    HDST_API
    HdBufferSourceSharedPtrVector GetPreChainedBuffers() const override;

    HDST_API
    bool HasChainedBuffer() const override;

    HDST_API
    HdBufferSourceSharedPtrVector GetChainedBuffers() const override;

protected:
    HDST_API
    bool _CheckValid() const override;

private:
    void _ResolveTris();
    void _ResolveQuads();

    HdBufferSourceSharedPtr _indices;
    HdBufferSourceSharedPtr _edgeIndices;
    HdBufferSourceSharedPtr _primitiveParam;
    HdBufferSourceSharedPtrVector _fvarIndices;

    HdBufferSourceSharedPtr _lineEdgeIndices;
    HdBufferSourceSharedPtr _linePrimitiveParam;
    HdBufferSourceSharedPtrVector _lineFvarIndices;

    bool _refined{};
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_WIREFRAME_LINES_COMPUTATION_H
