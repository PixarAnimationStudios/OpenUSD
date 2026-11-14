//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pySpan.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/init.hpp"

#include <numeric>
#include <string>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

// A minimal container to register as a span source.
class Tf_TestPySpanContainer
{
public:
    using value_type = int;

    explicit Tf_TestPySpanContainer(size_t n) : _elems(n) {
        std::iota(_elems.begin(), _elems.end(), 0);
    }

    int *data() { return _elems.data(); }
    int const *data() const { return _elems.data(); }
    size_t size() const { return _elems.size(); }

    size_t GetAddress() const { return reinterpret_cast<size_t>(data()); }
    list GetElements() const { return TfPyCopySequenceToList(_elems); }

private:
    std::vector<int> _elems;
};

// A source with no container interface that converts to TfSpan.
class Tf_TestPySpanConvertible
{
public:
    explicit Tf_TestPySpanConvertible(size_t n) : _elems(n) {
        std::iota(_elems.begin(), _elems.end(), 0.0);
    }

    operator TfSpan<double>() { return TfSpan<double>(_elems); }
    operator TfSpan<const double>() const {
        return TfSpan<const double>(_elems);
    }

    size_t GetAddress() const {
        return reinterpret_cast<size_t>(_elems.data());
    }
    list GetElements() const { return TfPyCopySequenceToList(_elems); }

private:
    std::vector<double> _elems;
};

// A source that permits only const access to its elements.
class Tf_TestPySpanReadOnly
{
public:
    explicit Tf_TestPySpanReadOnly(size_t n) : _elems(n) {
        std::iota(_elems.begin(), _elems.end(), 0);
    }

    operator TfSpan<const int>() const { return TfSpan<const int>(_elems); }

    size_t GetAddress() const {
        return reinterpret_cast<size_t>(_elems.data());
    }

private:
    std::vector<int> _elems;
};

template <class T>
size_t
_GetAddress(TfSpan<T> span)
{
    return reinterpret_cast<size_t>(span.data());
}

template <class T>
list
_GetElements(TfSpan<const T> span)
{
    return TfPyCopySequenceToList(span);
}

template <class T>
list
_GetElementsByConstRef(TfSpan<const T> const &span)
{
    return TfPyCopySequenceToList(span);
}

template <class T>
void
_Fill(TfSpan<T> span, T const &value)
{
    std::fill(span.begin(), span.end(), value);
}

class Tf_TestPySpan
{
public:
    static void Init() {
        // Keep this registration out of non-test sessions.
        TfPyRegisterSpanSource<Tf_TestPySpanContainer, int>();
        TfPyRegisterSpanSource<Tf_TestPySpanConvertible, double>();
        TfPyRegisterSpanSource<Tf_TestPySpanReadOnly, const int>();
    }
};

} // anonymous namespace

void wrapTf_TestPySpan()
{
    using This = Tf_TestPySpan;

    class_<Tf_TestPySpanContainer>("Tf_TestPySpanContainer", init<size_t>())
        .def("GetAddress", &Tf_TestPySpanContainer::GetAddress)
        .def("GetElements", &Tf_TestPySpanContainer::GetElements)
        ;

    class_<Tf_TestPySpanConvertible>(
        "Tf_TestPySpanConvertible", init<size_t>())
        .def("GetAddress", &Tf_TestPySpanConvertible::GetAddress)
        .def("GetElements", &Tf_TestPySpanConvertible::GetElements)
        ;

    class_<Tf_TestPySpanReadOnly>("Tf_TestPySpanReadOnly", init<size_t>())
        .def("GetAddress", &Tf_TestPySpanReadOnly::GetAddress)
        ;

    class_<This, noncopyable>("Tf_TestPySpan", no_init)
        .def("Init", &This::Init)
        .staticmethod("Init")

        .def("GetIntAddress", &_GetAddress<const int>)
        .staticmethod("GetIntAddress")
        .def("GetIntElements", &_GetElements<int>)
        .staticmethod("GetIntElements")
        .def("GetIntElementsByConstRef", &_GetElementsByConstRef<int>)
        .staticmethod("GetIntElementsByConstRef")
        .def("FillInts", &_Fill<int>)
        .staticmethod("FillInts")

        .def("GetDoubleAddress", &_GetAddress<const double>)
        .staticmethod("GetDoubleAddress")
        .def("GetDoubleElements", &_GetElements<double>)
        .staticmethod("GetDoubleElements")
        .def("FillDoubles", &_Fill<double>)
        .staticmethod("FillDoubles")

        .def("GetBoolAddress", &_GetAddress<const bool>)
        .staticmethod("GetBoolAddress")

        .def("GetStringElements", &_GetElements<std::string>)
        .staticmethod("GetStringElements")
        ;
}
