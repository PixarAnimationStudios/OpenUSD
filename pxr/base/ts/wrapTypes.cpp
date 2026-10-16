//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/optional.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

static
nb::list _WrapSplineSamplesPolylines(const TsSplineSamples<GfVec2d>& samples)
{
    TfPyLock lock;
    nb::list pyPolylines;
    for (const auto& polyline : samples.polylines) {
        nb::list pyPolyline;
        for (const auto& vertex : polyline) {
            pyPolyline.append(nb::cast(vertex));
        }
        pyPolylines.append(pyPolyline);
    }
    return pyPolylines;
}

static
nb::list _WrapSplineSamplesWithSourcesPolylines(
    const TsSplineSamplesWithSources<GfVec2d>& samples)
{
    TfPyLock lock;
    nb::list pyPolylines;
    for (const auto& polyline : samples.polylines) {
        nb::list pyPolyline;
        for (const auto& vertex : polyline) {
            pyPolyline.append(nb::cast(vertex));
        }
        pyPolylines.append(pyPolyline);
    }
    return pyPolylines;
}

static
nb::list _WrapSplineSamplesWithSourcesSources(
    const TsSplineSamplesWithSources<GfVec2d>& samples)
{
    return TfNbSequenceToList(samples.sources);
}

void wrapSplineSamples(nanobind::module_& m)
{
    nb::class_<TsSplineSamples<GfVec2d>>(m, "SplineSamples")
        .def_prop_ro("polylines", &_WrapSplineSamplesPolylines)
        ;
}

void wrapSplineSamplesWithSources(nanobind::module_& m)
{
    nb::class_<TsSplineSamplesWithSources<GfVec2d>>(
        m, "SplineSamplesWithSources")
        .def_prop_ro("polylines", &_WrapSplineSamplesWithSourcesPolylines)
        .def_prop_ro("sources", &_WrapSplineSamplesWithSourcesSources)
        ;
}

void wrapTypes(nanobind::module_& m)
{
    namespace nb = nanobind;
    TfPyWrapEnum<TsInterpMode>();
    TfPyWrapEnum<TsCurveType>();
    TfPyWrapEnum<TsExtrapMode>();
    TfPyWrapEnum<TsAntiRegressionMode>();
    TfPyWrapEnum<TsSplineSampleSource>();
    TfPyWrapEnum<TsTangentAlgorithm>();

    nb::class_<Ts_LoopParams>(m, "_LoopParams")
        .def(nb::init<>())
        .def(nb::init<const Ts_LoopParams &>())
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def_rw("protoStart", &Ts_LoopParams::protoStart)
        .def_rw("protoEnd", &Ts_LoopParams::protoEnd)
        .def_rw("numPreLoops", &Ts_LoopParams::numPreLoops)
        .def_rw("numPostLoops", &Ts_LoopParams::numPostLoops)
        .def_rw("valueOffset", &Ts_LoopParams::valueOffset)

        .def("GetPrototypeInterval", &Ts_LoopParams::GetPrototypeInterval)
        .def("GetLoopedInterval", &Ts_LoopParams::GetLoopedInterval)

        ;

    nb::class_<TsExtrapolation>(m, "Extrapolation")
        .def(nb::init<>())
        .def(nb::init_implicit<TsExtrapMode>())
        .def(nb::init<const TsExtrapolation &>())
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def_rw("mode", &TsExtrapolation::mode)
        .def_rw("slope", &TsExtrapolation::slope)
        .def_rw("loopBoundaryTime", &TsExtrapolation::loopBoundaryTime)

        .def("IsLooping", &TsExtrapolation::IsLooping)

        ;

    wrapSplineSamples(m);
    wrapSplineSamplesWithSources(m);
}
