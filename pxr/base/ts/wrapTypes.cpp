//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/tf/pyEnum.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;


void wrapTypes()
{
    TfPyWrapEnum<TsInterpMode>();
    TfPyWrapEnum<TsCurveType>();
    TfPyWrapEnum<TsExtrapMode>();
    TfPyWrapEnum<TsAntiRegressionMode>();

    class_<TsLoopParams>("LoopParams")

        // Default init is not suppressed, so automatically generated.

        .def(init<const TsLoopParams &>())
        .def(self == self)
        .def(self != self)

        .def_readwrite("protoStart", &TsLoopParams::protoStart)
        .def_readwrite("protoEnd", &TsLoopParams::protoEnd)
        .def_readwrite("numPreLoops", &TsLoopParams::numPreLoops)
        .def_readwrite("numPostLoops", &TsLoopParams::numPostLoops)
        .def_readwrite("valueOffset", &TsLoopParams::valueOffset)

        .def("GetPrototypeInterval", &TsLoopParams::GetPrototypeInterval)
        .def("GetLoopedInterval", &TsLoopParams::GetLoopedInterval)

        ;

    class_<TsExtrapolation>("Extrapolation")

        // Default init is not suppressed, so automatically generated.

        .def(init<TsExtrapMode>())
        .def(init<const TsExtrapolation &>())
        .def(self == self)
        .def(self != self)

        .def_readwrite("mode", &TsExtrapolation::mode)
        .def_readwrite("slope", &TsExtrapolation::slope)

        .def("IsLooping", &TsExtrapolation::IsLooping)

        ;
}
