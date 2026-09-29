//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyOptional.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/operators.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static
object _WrapSplineSamplesPolylines(const TsSplineSamples<GfVec2d>& samples)
{
    TfPyLock lock;
    pxr_boost::python::list pyPolylines;
    for (const auto& polyline : samples.polylines) {
        pxr_boost::python::list pyPolyline;
        for (const auto& vertex : polyline) {
            pyPolyline.append(vertex);
        }
        pyPolylines.append(pyPolyline);
    }
    return pyPolylines;
}

static
object _WrapSplineSamplesWithSourcesPolylines(
    const TsSplineSamplesWithSources<GfVec2d>& samples)
{
    TfPyLock lock;
    pxr_boost::python::list pyPolylines;
    for (const auto& polyline : samples.polylines) {
        pxr_boost::python::list pyPolyline;
        for (const auto& vertex : polyline) {
            pyPolyline.append(vertex);
        }
        pyPolylines.append(pyPolyline);
    }
    return pyPolylines;
}

static
object _WrapSplineSamplesWithSourcesSources(
    const TsSplineSamplesWithSources<GfVec2d>& samples)
{
    return TfPyCopySequenceToList(samples.sources);
}

void wrapSplineSamples()
{
    class_<TsSplineSamples<GfVec2d>>("SplineSamples", no_init)

        .add_property("polylines", &_WrapSplineSamplesPolylines)

        ;
}

void wrapSplineSamplesWithSources()
{
    class_<TsSplineSamplesWithSources<GfVec2d>>("SplineSamplesWithSources", no_init)

        .add_property("polylines", &_WrapSplineSamplesWithSourcesPolylines)
        .add_property("sources", &_WrapSplineSamplesWithSourcesSources)

        ;
}

void wrapTypes()
{
    TfPyWrapEnum<TsInterpMode>();
    TfPyWrapEnum<TsCurveType>();
    TfPyWrapEnum<TsExtrapMode>();
    TfPyWrapEnum<TsAntiRegressionMode>();
    TfPyWrapEnum<TsSplineSampleSource>();
    TfPyWrapEnum<TsTangentAlgorithm>();

    class_<Ts_LoopParams>("_LoopParams")

        // Default init is not suppressed, so automatically generated.

        .def(init<const Ts_LoopParams &>())
        .def(self == self)
        .def(self != self)

        .def_readwrite("protoStart", &Ts_LoopParams::protoStart)
        .def_readwrite("protoEnd", &Ts_LoopParams::protoEnd)
        .def_readwrite("numPreLoops", &Ts_LoopParams::numPreLoops)
        .def_readwrite("numPostLoops", &Ts_LoopParams::numPostLoops)
        .def_readwrite("valueOffset", &Ts_LoopParams::valueOffset)

        .def("GetPrototypeInterval", &Ts_LoopParams::GetPrototypeInterval)
        .def("GetLoopedInterval", &Ts_LoopParams::GetLoopedInterval)

        ;

    class_<TsExtrapolation>("Extrapolation")

        // Default init is not suppressed, so automatically generated.

        .def(init<TsExtrapMode>())
        .def(init<const TsExtrapolation &>())
        .def(self == self)
        .def(self != self)

        .def_readwrite("mode", &TsExtrapolation::mode)
        .def_readwrite("slope", &TsExtrapolation::slope)

        // def_readwrite expects a registered Python class for the member's
        // type to return a reference on read, but loopBoundaryTime is an
        // optional. So we manually register the read/write.
        .add_property("loopBoundaryTime",
            +[](const TsExtrapolation& self) {
                return self.loopBoundaryTime;
            },
            +[](TsExtrapolation& self, const std::optional<double>& value) {
                self.loopBoundaryTime = value;
            })

        .def("IsLooping", &TsExtrapolation::IsLooping)

        ;

    wrapSplineSamples();
    wrapSplineSamplesWithSources();

}
