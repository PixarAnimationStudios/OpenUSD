//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/timeCodeRange.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/usd/usd/timeCode.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>

#include <sstream>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

static
std::string
_FrameSpec(const UsdUtilsTimeCodeRange& timeCodeRange)
{
    std::ostringstream ostream;
    ostream << timeCodeRange;
    return ostream.str();
}

static
std::string
_Repr(const UsdUtilsTimeCodeRange& timeCodeRange)
{
    if (timeCodeRange.empty()) {
        return TF_PY_REPR_PREFIX + "TimeCodeRange()";
    }

    return TF_PY_REPR_PREFIX + "TimeCodeRange.CreateFromFrameSpec('" +
            _FrameSpec(timeCodeRange) + "')";
}

class UsdUtils_PyTimeCodeRangeIterator
{
public:
    explicit UsdUtils_PyTimeCodeRangeIterator(
            const UsdUtilsTimeCodeRange& timeCodeRange) :
        _iter(timeCodeRange.begin()),
        _end(timeCodeRange.end()),
        _currTimeCode(_iter != _end ? *_iter : UsdTimeCode()),
        _didFirst(false)
    {
    }

    UsdUtils_PyTimeCodeRangeIterator iter(
            const UsdUtils_PyTimeCodeRangeIterator& iter)
    {
        return iter;
    }

    UsdTimeCode next() {
        _RaiseIfAtEnd();

        if (_didFirst) {
            ++_iter;
            _RaiseIfAtEnd();
        }

        _didFirst = true;
        _currTimeCode = *_iter;
        return _currTimeCode;
    }

private:

    void _RaiseIfAtEnd() const {
        if (_iter == _end) {
            throw nb::stop_iteration("UsdUtilsTimeCodeRange at end");
        }
    }

    UsdUtilsTimeCodeRange::iterator _iter;
    const UsdUtilsTimeCodeRange::iterator _end;
    UsdTimeCode _currTimeCode;
    bool _didFirst;
};

static
UsdUtils_PyTimeCodeRangeIterator
UsdUtils_PyTimeCodeRangeIteratorCreate(
        const UsdUtilsTimeCodeRange& timeCodeRange)
{
    return UsdUtils_PyTimeCodeRangeIterator(timeCodeRange);
}

} // anonymous namespace

void wrapTimeCodeRange(nanobind::module_ &m)
{
    using This = UsdUtilsTimeCodeRange;

    nb::class_<This> s(m, "TimeCodeRange");
    s.def(nb::init<>())
        .def(nb::init<UsdTimeCode>(), nb::arg("timeCode"))
        .def(nb::init<UsdTimeCode, UsdTimeCode>(),
            nb::arg("startTimeCode"), nb::arg("endTimeCode"))
        .def(nb::init<UsdTimeCode, UsdTimeCode, double>(),
            nb::arg("startTimeCode"), nb::arg("endTimeCode"), nb::arg("stride"))
        .def_static("CreateFromFrameSpec", &This::CreateFromFrameSpec)
        .def_prop_ro("startTimeCode", &This::GetStartTimeCode)
        .def_prop_ro("endTimeCode", &This::GetEndTimeCode)
        .def_prop_ro("stride", &This::GetStride)
        .def_prop_ro("frameSpec", _FrameSpec)

        .def("empty", &This::empty)
        .def("IsValid", &This::IsValid)

        .def("__bool__", [](const This &self) { return static_cast<bool>(self); })
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        .def("__repr__", _Repr)
        .def("__iter__", &UsdUtils_PyTimeCodeRangeIteratorCreate,
            nb::keep_alive<0, 1>())
        ;

    TF_NB_WRAP_PUBLIC_TOKENS(
        s,
        "Tokens",
        UsdUtilsTimeCodeRangeTokens,
        USDUTILS_TIME_CODE_RANGE_TOKENS);

    nb::class_<UsdUtils_PyTimeCodeRangeIterator>(s, "_Iterator")
        .def("__iter__", [](nb::handle self) { return self; })
        .def("__next__", &UsdUtils_PyTimeCodeRangeIterator::next,
            nb::rv_policy::copy)
        ;
}
