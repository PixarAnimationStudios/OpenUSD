//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_SampleTimes.h"
#include "pxr/base/ts/tsTest_SplineData.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"

#include <boost/python.hpp>
#include <sstream>
#include <string>
#include <cstdio>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

using This = TsTest_SampleTimes;


// Return a full-precision python repr for a double value.
static std::string
_HexFloatRepr(const double num)
{
    // XXX: work around std::hexfloat apparently not working in our libstdc++ as
    // of this writing.
    char buf[100];
    sprintf(buf, "float.fromhex('%a')", num);
    return std::string(buf);
}

static std::string
_SampleTimeRepr(const This::SampleTime &st)
{
    std::ostringstream result;

    result << "Ts.TsTest_SampleTimes.SampleTime("
           << _HexFloatRepr(st.time)
           << ", " << st.pre
           << ")";

    return result.str();
}

static This*
_ConstructSampleTimes(
    const object &times)
{
    This *result = new This();

    std::vector<This::SampleTime> sts;
    if (!times.is_none())
    {
        extract<std::vector<This::SampleTime>> extractor(times);
        if (extractor.check())
            sts = extractor();
        else
            TF_CODING_ERROR("Unexpected type for times");
    }
    result->AddTimes(sts);

    return result;
}

static std::string
_SampleTimesRepr(const This &times)
{
    std::ostringstream result;

    std::vector<std::string> stStrs;
    for (const This::SampleTime &st : times.GetTimes())
        stStrs.push_back(_SampleTimeRepr(st));

    result << "Ts.TsTest_SampleTimes([" << TfStringJoin(stStrs, ", ") << "])";

    return result.str();
}


void wrapTsTest_SampleTimes()
{
    // First the class object, so we can create a scope for it...
    class_<This> classObj("TsTest_SampleTimes", no_init);
    scope classScope(classObj);

    // ...then the nested type wrappings, which require the scope...

    class_<This::SampleTime>("SampleTime")
        // Note default init is not suppressed, so automatically created.
        .def(init<double>())
        .def(init<double, bool>())
        .def(init<const This::SampleTime&>())
        .def("__repr__", &_SampleTimeRepr)
        .def(self < self)
        .def(self == self)
        .def(self != self)
        .def_readwrite("time", &This::SampleTime::time)
        .def_readwrite("pre", &This::SampleTime::pre)
        ;

    TfPyRegisterStlSequencesFromPython<double>();
    TfPyRegisterStlSequencesFromPython<This::SampleTime>();

    // ...then the defs, which must occur after the nested type wrappings.
    classObj

        // This serves as a default constructor and a from-repr constructor.
        .def("__init__",
            make_constructor(
                &_ConstructSampleTimes, default_call_policies(),
                (arg("times") = object())))

        .def("__repr__", &_SampleTimesRepr)

        .def("AddTimes",
            static_cast<void (This::*)(const std::vector<double>&)>(
                &This::AddTimes))

        .def("AddTimes",
            static_cast<void (This::*)(const std::vector<This::SampleTime>&)>(
                &This::AddTimes))

        .def(init<const TsTest_SplineData&>())

        .def("AddKnotTimes", &This::AddKnotTimes)

        .def("AddUniformInterpolationTimes",
            &This::AddUniformInterpolationTimes,
            (arg("numSamples")))

        .def("AddExtrapolationTimes", &This::AddExtrapolationTimes,
            (arg("extrapolationFactor")))

        .def("AddStandardTimes", &This::AddStandardTimes)

        .def("GetTimes", &This::GetTimes,
            return_value_policy<TfPySequenceToList>())

        ;
}
