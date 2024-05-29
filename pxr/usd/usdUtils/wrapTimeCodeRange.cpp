//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/timeCodeRange.h"

#include "pxr/base/tf/pyStaticTokens.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/usd/usd/timeCode.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/object.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/with_custodian_and_ward.hpp>

#include <sstream>
#include <string>


using namespace boost::python;


PXR_NAMESPACE_USING_DIRECTIVE


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
            PyErr_SetString(
                PyExc_StopIteration,
                "UsdUtilsTimeCodeRange at end");
            throw_error_already_set();
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


void wrapTimeCodeRange()
{
    using This = UsdUtilsTimeCodeRange;

    scope s = class_<This>("TimeCodeRange")
        .def(init<UsdTimeCode>(arg("timeCode")))
        .def(init<UsdTimeCode, UsdTimeCode>(
            (arg("startTimeCode"), arg("endTimeCode"))))
        .def(init<UsdTimeCode, UsdTimeCode, double>(
            (arg("startTimeCode"), arg("endTimeCode"), arg("stride"))))
        .def("CreateFromFrameSpec", &This::CreateFromFrameSpec)
            .staticmethod("CreateFromFrameSpec")
        .add_property("startTimeCode", &This::GetStartTimeCode)
        .add_property("endTimeCode", &This::GetEndTimeCode)
        .add_property("stride", &This::GetStride)
        .add_property("frameSpec", _FrameSpec)

        .def("empty", &This::empty)
        .def("IsValid", &This::IsValid)

        .def(!self)
        .def(self == self)
        .def(self != self)
        .def("__repr__", _Repr)
        .def("__iter__", &UsdUtils_PyTimeCodeRangeIteratorCreate,
            with_custodian_and_ward_postcall<0, 1>())
        ;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "Tokens",
        UsdUtilsTimeCodeRangeTokens,
        USDUTILS_TIME_CODE_RANGE_TOKENS);

    class_<UsdUtils_PyTimeCodeRangeIterator>("_Iterator", no_init)
        .def("__iter__", &UsdUtils_PyTimeCodeRangeIterator::iter,
            return_self<>())
        .def("__next__", &UsdUtils_PyTimeCodeRangeIterator::next,
            return_value_policy<return_by_value>())
        ;
}
