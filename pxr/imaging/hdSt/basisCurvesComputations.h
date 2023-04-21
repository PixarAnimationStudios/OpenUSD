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
#ifndef PXR_IMAGING_HD_ST_BASIS_CURVES_COMPUTATIONS_H
#define PXR_IMAGING_HD_ST_BASIS_CURVES_COMPUTATIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hdSt/basisCurvesTopology.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/vec3f.h"

#include <algorithm>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdSt_BasisCurvesIndexBuilderComputation
///
/// Compute basis curves indices as a computation on CPU.
///
class HdSt_BasisCurvesIndexBuilderComputation : public HdComputedBufferSource {
public:
    HdSt_BasisCurvesIndexBuilderComputation(HdBasisCurvesTopology *topology,
                                            bool forceLines);
    virtual void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    virtual bool Resolve() override;

    virtual bool HasChainedBuffer() const override;
    virtual HdBufferSourceSharedPtrVector GetChainedBuffers() const override;

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


template <typename T> 
VtArray<T>
HdSt_ExpandVarying(const SdfPath &id, const TfToken &name,
    size_t numVerts, VtIntArray const &vertexCounts, 
    const TfToken &wrap, const TfToken &basis, 
    VtArray<T> const &authoredValues, const T fallbackValue)
{
    VtArray<T> outputValues(numVerts);

    size_t srcIndex = 0;
    size_t dstIndex = 0;

    if (wrap == HdTokens->periodic) {
        // XXX(HYD-2238): Add support for periodic curves.
        TF_WARN("HdStBasisCurves(%s) - Periodic expansion hasn't been"
                " implemented; expanding primvar %s as if non-periodic.",
                id.GetText(), name.GetText());
    }

    if (basis == HdTokens->catmullRom || 
        basis == HdTokens->centripetalCatmullRom ||
        basis == HdTokens->bspline) {
        for (const int nVerts : vertexCounts) {
            // Handling for the case of potentially incorrect vertex counts 
            if (nVerts < 1) {
                continue;
            }

            // For splines with a vstep of 1, we are doing linear interpolation 
            // between segments, so all we do here is duplicate the first and 
            // last outputValues. Since these are never acutally used during 
            // drawing, it would also work just to set the value to 0.
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex;
            for (int i = 1; i < nVerts - 2; ++i){
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++dstIndex; ++srcIndex;
            }
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex;
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; ++srcIndex;
        }
        TF_VERIFY(srcIndex == authoredValues.size());
        TF_VERIFY(dstIndex == numVerts);
    } else if (basis == HdTokens->bezier) {
        for (const int nVerts : vertexCounts) {
            // Handling for the case of potentially incorrect vertex counts 
            if (nVerts < 1) {
                continue;
            }

            // For bezier splines, we map the linear values to cubic values
            // the begin value gets mapped to the first two vertices and
            // the end value gets mapped to the last two vertices in a segment.
            // shaders can choose to access value[1] and value[2] when linearly
            // interpolating a value, which happens to match up with the
            // indexing to use for catmullRom and bSpline basis.
            const int vStep = 3;
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; // don't increment the srcIndex
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; ++ srcIndex;

            // vstep - 1 control points will have an interpolated value
            for(int i = 2; i < nVerts - 2; i += vStep) {
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++ dstIndex; // don't increment the srcIndex
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++ dstIndex; // don't increment the srcIndex
                outputValues[dstIndex] = authoredValues[srcIndex];
                ++ dstIndex; ++ srcIndex; 
            }
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; // don't increment the srcIndex
            outputValues[dstIndex] = authoredValues[srcIndex];
            ++dstIndex; ++ srcIndex;
        }
        TF_VERIFY(srcIndex == authoredValues.size());
        TF_VERIFY(dstIndex == numVerts);
    } else {
        std::fill(outputValues.begin(), outputValues.end(), fallbackValue);
        TF_WARN("HdStBasisCurves(%s) - Varying interpolation of primvar %s has"
                " unsupported basis %s, using fallback value for rendering",
                id.GetText(), name.GetText(), basis.GetText());
    }

    return outputValues;
}


/// Verify the number of authored vertex or varying primvars, expanding the 
/// number of varying values when necessary
template <typename T>
class HdSt_BasisCurvesPrimvarInterpolaterComputation
    : public HdComputedBufferSource {
public:
    HdSt_BasisCurvesPrimvarInterpolaterComputation(
        HdSt_BasisCurvesTopologySharedPtr topology,
        const VtArray<T> &authoredPrimvar,
        const SdfPath &id,
        const TfToken &name,
        HdInterpolation interpolation,
        const T fallbackValue,
        HdType hdType) 
    : _topology(topology)
    , _authoredPrimvar(authoredPrimvar)
    , _id(id)
    , _name(name)
    , _interpolation(interpolation)
    , _fallbackValue(fallbackValue)
    , _hdType(hdType)
{}

    virtual bool Resolve() override {
        if (!_TryLock()) return false;

        HD_TRACE_FUNCTION();

        // Varying primvars are expanded to per-vertex, so the expected vertex
        // primvar size is used below.
        const size_t numVertsExpected =
            _topology->CalculateNeededNumberOfControlPoints();
        VtArray<T> primvars(numVertsExpected);
        const size_t authoredSize = _authoredPrimvar.size();

        // Special handling for when points is size 0
        if (authoredSize == 0 && _name == HdTokens->points) {
            primvars = _authoredPrimvar;

        } else if (_interpolation == HdInterpolationVertex) {

            if (authoredSize == numVertsExpected) {
                primvars = _authoredPrimvar;

            } else if (authoredSize == 1) {
                // Treat it as a constant primvar.
                std::fill(primvars.begin(), primvars.end(),
                          _authoredPrimvar[0]);

            } else if (_topology->HasIndices() &&
                       authoredSize > numVertsExpected) {
                // When indices are supplied and don't cover the length of the
                // authored primvar (e.g., we have 10 points but the indices
                // reference upto 7), truncate the primvar to that referenced by
                // the indices.
                // Note that the underspecified scenario (wherein the authored
                // primvar size is lesser than the expectation) gets the
                // fallback treatment in the else clause below.
                primvars = _authoredPrimvar;
                primvars.resize(numVertsExpected);

            } else {
                std::fill(primvars.begin(), primvars.end(), _fallbackValue);

                std::stringstream s;
                s << "HdStBasisCurves(" << _id.GetText() << ")"
                  << "- Primvar " <<  _name.GetText()
                  << " has incorrect size for vertex interpolation "
                  << "(need " << numVertsExpected << ", got " << authoredSize
                  << "), using fallback value " << _fallbackValue
                  << " for rendering.";
                
                TF_WARN(s.str());
            }

        } else if (_interpolation == HdInterpolationVarying) {

            const size_t numVaryingExpected =
                _topology->CalculateNeededNumberOfVaryingControlPoints();

            if (authoredSize == numVaryingExpected) {
                if (_topology->GetCurveType() == HdTokens->linear) {
                    // Varying primvars are specified per-vertex for linear.
                    primvars = _authoredPrimvar;

                } else {
                    // Expand the authored primvar to per-vertex.
                    primvars = HdSt_ExpandVarying<T>(
                        _id, _name, numVertsExpected,
                        _topology->GetCurveVertexCounts(),
                        _topology->GetCurveWrap(), _topology->GetCurveBasis(),
                        _authoredPrimvar, _fallbackValue);
                }

            } else if (authoredSize == 1) {
                // Treat it as a constant primvar.
                std::fill(primvars.begin(), primvars.end(),
                          _authoredPrimvar[0]);

            } else {
                std::fill(primvars.begin(), primvars.end(), _fallbackValue);

                std::stringstream s;
                s << "HdStBasisCurves(" << _id.GetText() << ")"
                  << "- Primvar " <<  _name.GetText()
                  << " has incorrect size for varying interpolation "
                  << "(need " << numVaryingExpected << ", got " << authoredSize
                  << "), using fallback value " << _fallbackValue
                  << " for rendering.";
                
                TF_WARN(s.str());
            }
        }

        _SetResult(std::make_shared<HdVtBufferSource>(
            _name, VtValue(primvars)));

        _SetResolved();
        return true;
    }

    virtual void GetBufferSpecs(HdBufferSpecVector *specs) const override {
        specs->emplace_back(_name, HdTupleType{_hdType, 1});
    }

protected:
    virtual bool _CheckValid() const override {
        return true;
    }

private:
    HdSt_BasisCurvesTopologySharedPtr _topology;
    VtArray<T> _authoredPrimvar;
    SdfPath _id;
    TfToken _name;
    HdInterpolation _interpolation;
    T _fallbackValue;
    HdType _hdType;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_BASIS_CURVES_COMPUTATIONS_H
