//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PY_LIST_PROXY_H
#define PXR_USD_SDF_PY_LIST_PROXY_H

/// \file sdf/pyListProxy.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/listProxy.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include <stdexcept>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_OPEN_SCOPE

template <class T>
class SdfPyWrapListProxy {
public:
    typedef T Type;
    typedef typename Type::TypePolicy TypePolicy;
    typedef typename Type::value_type value_type;
    typedef typename Type::value_vector_type value_vector_type;
    typedef SdfPyWrapListProxy<Type> This;

    SdfPyWrapListProxy()
    {
        TfPyWrapOnce<Type>(&This::_Wrap);
    }

private:
    static void _Wrap()
    {
        namespace nb = nanobind;

        nb::handle scope = TfPyGetCurrentNanobindModule();
        if (!scope.is_valid()) {
            scope = nb::borrow(PyImport_AddModule("pxr.Sdf"));
        }

        nb::class_<Type>(scope, _GetName().c_str())
            .def("__str__", &This::_GetStr)
            .def("__len__", &Type::size)
            .def("__getitem__", &This::_GetItemIndex)
            .def("__getitem__", &This::_GetItemSlice)
            .def("__setitem__", &This::_SetItemIndex)
            .def("__setitem__", &This::_SetItemSlice)
            .def("__delitem__", &This::_DelItemIndex)
            .def("__delitem__", &This::_DelItemSlice)
            .def("__delitem__", &Type::Remove)
            .def("count", &Type::Count)
            .def("copy", [](const Type& x) {
                return static_cast<value_vector_type>(x);
            })
            .def("index", &This::_FindIndex)
            .def("clear", &Type::clear)
            .def("insert", &This::_Insert)
            .def("append", &Type::push_back)
            .def("remove", &Type::Remove)
            .def("replace", &Type::Replace)
            .def("ApplyList", &Type::ApplyList)
            .def("ApplyEditsToList", &This::_ApplyEditsToList)
            .def_prop_ro("expired", &This::_IsExpired)
            .def_prop_ro_static("invalidIndex", [](nb::handle) {
                return This::_GetInvalidIndex();
            })
            .def(nb::self == nb::self)
            .def(nb::self != nb::self)
            .def(nb::self <  nb::self)
            .def(nb::self <= nb::self)
            .def(nb::self >  nb::self)
            .def(nb::self >= nb::self)
            .def(nb::self == value_vector_type())
            .def(nb::self != value_vector_type())
            .def(nb::self <  value_vector_type())
            .def(nb::self <= value_vector_type())
            .def(nb::self >  value_vector_type())
            .def(nb::self >= value_vector_type())
            ;

    }

    static std::string _GetName()
    {
        std::string name = "ListProxy_" +
                           ArchGetDemangled<TypePolicy>();
        name = TfStringReplace(name, " ", "_");
        name = TfStringReplace(name, ",", "_");
        name = TfStringReplace(name, "::", "_");
        name = TfStringReplace(name, "<", "_");
        name = TfStringReplace(name, ">", "_");
        return name;
    }

    static std::string _GetStr(const Type& x)
    {
        return TfPyRepr(static_cast<value_vector_type>(x));
    }

    static value_type _GetItemIndex(const Type& x, int index)
    {
        return x[TfPyNormalizeIndex(index, x._GetSize(), true)];
    }

    static nanobind::list _GetItemSlice(const Type& x,
                                        const nanobind::slice& index)
    {
        nanobind::list result;

        if (x._Validate()) {
            auto [start, stop, step, count] = index.compute(x._GetSize());
            for (size_t i = 0; i < count; ++i, start += step) {
                result.append(nanobind::cast(x[start]));
            }
        }

        return result;
    }

    static void _SetItemIndex(Type& x, int index, const value_type& value)
    {
        x[TfPyNormalizeIndex(index, x._GetSize(), true)] = value;
    }

    static void _SetItemSlice(Type& x, const nanobind::slice& index,
                              const value_vector_type& values)
    {
        if (! x._Validate()) {
            return;
        }

        auto [start, stop, step, count] = index.compute(x._GetSize());
        if (count == 0) {
            int s = 0;
            if (nanobind::try_cast<int>(index.attr("start"), s)) {
                start = TfPyNormalizeIndex(s, x._GetSize(), true);
            } else {
                start = 0;
            }
            step = 0;
        }

        if (TfPyIsNone(index.attr("step"))) {
            // Replace contiguous sequence with values.
            x._Edit(static_cast<size_t>(start), count, values);
        }
        else {
            // Replace exactly the selected items.
            if (count != values.size()) {
                TfPyThrowValueError(
                    TfStringPrintf("attempt to assign sequence of size %zd "
                                   "to extended slice of size %zd",
                                   values.size(), count).c_str());
            }
            else if (step == 1) {
                x._Edit(static_cast<size_t>(start), count, values);
            }
            else {
                SdfChangeBlock block;
                for (size_t i = 0, j = static_cast<size_t>(start);
                     i != count; j += step, ++i) {
                    x._Edit(j, 1, value_vector_type(1, values[i]));
                }
            }
        }
    }

    static void _DelItemIndex(Type& x, int i)
    {
        x._Edit(TfPyNormalizeIndex(i, x._GetSize(), true),
                1, value_vector_type());
    }

    static void _DelItemSlice(Type& x, const nanobind::slice& index)
    {
        if (x._Validate()) {
            auto [start, stop, step, count] = index.compute(x._GetSize());
            if (count > 0) {
                // Erase items.
                if (step == 1) {
                    x._Edit(static_cast<size_t>(start), count,
                            value_vector_type());
                }
                else {
                    SdfChangeBlock block;
                    value_vector_type empty;
                    for (Py_ssize_t j = start; count > 0;
                         j += step - 1, --count) {
                        x._Edit(static_cast<size_t>(j), 1, empty);
                    }
                }
            }
        }
    }

    static int _GetInvalidIndex()
    {
        // Note that SdfListProxy::Find returns an unsigned value, however the 
        // wrapped class returns -1 in the event that a value could not be found
        // in the list of operations.
        return -1;
    }

    static int _FindIndex(const Type& x, const value_type& value)
    {
        if (x._Validate()) {
            const size_t index = x.Find(value);
            return index == Type::invalidIndex 
                ? _GetInvalidIndex() 
                : static_cast<int>(index); 
        }
        else {
            return _GetInvalidIndex();
        }
    }

    static void _Insert(Type& x, int index, const value_type& value)
    {
        if (index < 0) {
            index += x._GetSize();
        }
        if (index < 0 || index > static_cast<int>(x._GetSize())) {
            TfPyThrowIndexError("list index out of range");
        }
        x._Edit(index, 0, value_vector_type(1, value));
    }

    static bool _IsExpired(const Type& x)
    {
        return x.IsExpired();
    }

    static value_vector_type _ApplyEditsToList(Type& x, 
                                               const value_vector_type& values)
    {
        value_vector_type newValues = values;
        x.ApplyEditsToList(&newValues);
        return newValues;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PY_LIST_PROXY_H
