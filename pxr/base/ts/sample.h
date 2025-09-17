//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_SAMPLE_H
#define PXR_BASE_TS_SAMPLE_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/eval.h"
#include "pxr/base/ts/types.h"

PXR_NAMESPACE_OPEN_SCOPE

struct Ts_SplineData;

class Ts_SampleDataInterface
{
public:
    // Add a segment to the TsSplineSamples. If the vertex (time0, value0)
    // does not exactly mat with the last existing vertex in the polyline,
    // a new polyline will be started.
    virtual void
    AddSegment(double time0, double value0,
               double time1, double value1,
               TsSplineSampleSource source) = 0;

    // Clear the existing contents of the sample data prior to filling it.
    virtual void
    Clear() = 0;
};

template <typename T>
class Ts_SampleData : public Ts_SampleDataInterface
{
    Ts_SampleData(T* /* ignored */)
    {
        // It should not be possible to instantiate this base template.
        // Only the 2 partial specializations for TsSplineSamples and
        // TsSplineSamplesWithSources should be used.  Allow the class
        // to be constructed (it's not a static_assert), but fail a
        // TF_VERIFY and return an object that does nothing.
        TF_VERIFY(false, "Invalid splineSamples data type");
    }

    // This generic implementation of AddSegment does nothing. Use one
    // of the partial specializations below instead.
    void
    AddSegment(double time0, double value0,
               double time1, double value1,
               TsSplineSampleSource source) override
    { }

    // This generic implementation of Clear does nothing. Use one
    // of the partial specializations below instead.
    void
    Clear() override
    { }
    
};

// Partial specialization for TsSplineSamples
template <typename Vertex>
class Ts_SampleData<TsSplineSamples<Vertex>> : public Ts_SampleDataInterface
{
private:
    using SplineSamples = TsSplineSamples<Vertex>;
    TsSplineSamples<Vertex>* _sampledSpline;

public:
    Ts_SampleData(TsSplineSamples<Vertex>* sampledSpline)
    : _sampledSpline(sampledSpline)
    { }

    // Add a segment to the TsSplineSamples. If the vertex (time0, value0) does
    // not exactly match the last existing vertex in the polyline, a new
    // polyline will be started.
    void
    AddSegment(double time0, double value0,
               double time1, double value1,
               TsSplineSampleSource /* source*/) override
    {
        if (time0 > time1) {
            using std::swap;
            swap(time0, time1);
            swap(value0, value1);
        }
            
        Vertex vertex0(time0, value0);
        Vertex vertex1(time1, value1);

        if (_sampledSpline->polylines.empty() ||
            (!_sampledSpline->polylines.back().empty() &&
             _sampledSpline->polylines.back().back() != vertex0))
        {
            // We need to create a new polyline
            _sampledSpline->polylines.emplace_back(
                typename SplineSamples::Polyline{vertex0, vertex1});
        } else {
            _sampledSpline->polylines.back().push_back(vertex1);
        }
    }

    // Clear the existing contents of the TsSplineSamples prior to filling it.
    void
    Clear() override
    {
        _sampledSpline->polylines.clear();
    }
};

// Partial specialization for TsSplineSamplesWithSources
template <typename Vertex>
class Ts_SampleData<TsSplineSamplesWithSources<Vertex>> :
    public Ts_SampleDataInterface
{
private:
    using SplineSamples = TsSplineSamplesWithSources<Vertex>;
    TsSplineSamplesWithSources<Vertex>* _sampledSpline;

public:
    Ts_SampleData(TsSplineSamplesWithSources<Vertex>* sampledSpline)
    : _sampledSpline(sampledSpline)
    { }

    // Add a segment to the TsSplineSamplesWithSources. If the the source or the
    // vertex (time0, value0) does not exactly match the last existing polyline
    // source or vertex, a new source will be added and a new polyline will be
    // started.
    void
    AddSegment(double time0, double value0,
               double time1, double value1,
               TsSplineSampleSource source) override
    {
        if (time0 > time1) {
            using std::swap;
            swap(time0, time1);
            swap(value0, value1);
        }
            
        Vertex vertex0(time0, value0);
        Vertex vertex1(time1, value1);

        if (_sampledSpline->polylines.empty() ||
            _sampledSpline->sources.back() != source ||
            (!_sampledSpline->polylines.back().empty() &&
             _sampledSpline->polylines.back().back() != vertex0))
        {
            // We need to create a new polyline
            _sampledSpline->polylines.emplace_back(
                typename SplineSamples::Polyline{vertex0, vertex1});
            // And add a source for it.
            _sampledSpline->sources.push_back(source);
        } else {
            _sampledSpline->polylines.back().push_back(vertex1);
        }
    }

    void
    Clear() override
    {
        _sampledSpline->polylines.clear();
        _sampledSpline->sources.clear();
    }
};

// Note that SampleData will be some templated version of TsSplineSamples
// or TsSplineSamplesWithSources.
TS_API
void
Ts_Sample(const Ts_SplineData* data,
          const GfInterval& timeInterval,
          double timeScale,
          double valueScale,
          double tolerance,
          Ts_SampleDataInterface* sampledSpline);

#undef _INSTANTIATE_SAMPLE_METHOD

PXR_NAMESPACE_CLOSE_SCOPE

#endif
