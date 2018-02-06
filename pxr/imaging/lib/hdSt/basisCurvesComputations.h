//
// Copyright 2016 Pixar
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
#ifndef HDST_BASIS_CURVES_COMPUTATIONS_H
#define HDST_BASIS_CURVES_COMPUTATIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/vec3f.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdBasisCurvesTopology;

/// \class HdSt_BasisCurvesIndexBuilderComputation
///
/// Compute basis curves indices as a computation on CPU.
///
class HdSt_BasisCurvesIndexBuilderComputation : public HdComputedBufferSource {
public:
    HdSt_BasisCurvesIndexBuilderComputation(HdBasisCurvesTopology *topology,
                                            bool forceLines);
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const override;
    virtual bool Resolve() override;

    virtual bool HasChainedBuffer() const override;
    virtual HdBufferSourceSharedPtr GetChainedBuffer() const override;

protected:
    virtual bool _CheckValid() const override;

public:
    // For building index and primitive index arrays
    struct IndexAndPrimIndex {
        // default constructor results in empty VtValue's
        IndexAndPrimIndex() {}

        IndexAndPrimIndex(VtValue indices, VtValue primIndices) :
            _indices(indices), _primIndices(primIndices) {}

        VtValue _indices;
        VtValue _primIndices;
    };
private:
    IndexAndPrimIndex _BuildLinesIndexArray();
    IndexAndPrimIndex _BuildLineSegmentIndexArray();
    IndexAndPrimIndex _BuildCubicIndexArray();
                                    
    HdBasisCurvesTopology *_topology;
    bool _forceLines;

    HdBufferSourceSharedPtr _primitiveParam;    
};

/// Compute vertex widths based on \p authoredWidths, doing interpolation as
/// necessary 
///
class HdSt_BasisCurvesWidthsInterpolaterComputation
                            : public HdComputedBufferSource {
public:
    HdSt_BasisCurvesWidthsInterpolaterComputation(HdBasisCurvesTopology *topology,
                                                VtFloatArray authoredWidths);
    virtual bool Resolve();
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;

protected:
    virtual bool _CheckValid() const;

private:
    HdBasisCurvesTopology *_topology;
    VtFloatArray _authoredWidths;
};

/// Compute varying normals based on \p authoredNormals, doing interpolation as
/// necessary 
///
class HdSt_BasisCurvesNormalsInterpolaterComputation
                            : public HdComputedBufferSource {
public:
    HdSt_BasisCurvesNormalsInterpolaterComputation(HdBasisCurvesTopology *topology,
                                                VtVec3fArray authoredNormals);
    virtual bool Resolve();
    virtual void AddBufferSpecs(HdBufferSpecVector *specs) const;

protected:
    virtual bool _CheckValid() const;

private:
    HdBasisCurvesTopology *_topology;
    VtVec3fArray _authoredNormals;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDST_BASIS_CURVES_COMPUTATIONS_H
