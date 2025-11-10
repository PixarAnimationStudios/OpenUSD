//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DASH_DOT_LINES_COMPUTATIONS_H
#define PXR_IMAGING_HD_ST_DASH_DOT_LINES_COMPUTATIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hdSt/dashDotLinesTopology.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/vec3f.h"

#include <algorithm>
#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


/// \class HdSt_DashDotLinesIndexBuilderComputation
///
/// Compute dashdot lines indices as a computation on CPU.
///
class HdSt_DashDotLinesIndexBuilderComputation : public HdComputedBufferSource {
public:
    HdSt_DashDotLinesIndexBuilderComputation(HdDashDotLinesTopology *topology,
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
    IndexAndPrimIndex _BuildLineSegmentIndexArray(bool needAdjInfo);
                                    
    HdDashDotLinesTopology *_topology;
    bool _forceLines;

    HdBufferSourceSharedPtr _primitiveParam;    
};


/// Verify the number of authored vertex or varying primvars, expanding the 
/// number of varying values when necessary
template <typename T>
class HdSt_DashDotLinesPrimvarInterpolaterComputation
    : public HdComputedBufferSource {
public:
    HdSt_DashDotLinesPrimvarInterpolaterComputation(
        HdSt_DashDotLinesTopologySharedPtr topology,
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
                s << "HdStDashDotLines(" << _id.GetText() << ")"
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
                // Varying primvars are specified per-vertex for linear.
                primvars = _authoredPrimvar;
            } else if (authoredSize == 1) {
                // Treat it as a constant primvar.
                std::fill(primvars.begin(), primvars.end(),
                          _authoredPrimvar[0]);

            } else {
                std::fill(primvars.begin(), primvars.end(), _fallbackValue);

                std::stringstream s;
                s << "HdStDashDotLines(" << _id.GetText() << ")"
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
    HdSt_DashDotLinesTopologySharedPtr _topology;
    VtArray<T> _authoredPrimvar;
    SdfPath _id;
    TfToken _name;
    HdInterpolation _interpolation;
    T _fallbackValue;
    HdType _hdType;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_DASH_DOT_LINES_COMPUTATIONS_H
