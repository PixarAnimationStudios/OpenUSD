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
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"
#include <stdexcept>
#include "pxr/external/boost/python.hpp"
#include "pxr/external/boost/python/slice.hpp"

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
        using namespace pxr_boost::python;

        class_<Type>(_GetName().c_str(), no_init)
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
            .def("copy", &Type::operator value_vector_type,
                 return_value_policy<TfPySequenceToList>())
            .def("index", &This::_FindIndex)
            .def("clear", &Type::clear)
            .def("insert", &This::_Insert)
            .def("append", &Type::push_back)
            .def("remove", &Type::Remove)
            .def("replace", &Type::Replace)
            .def("ApplyList", &Type::ApplyList)
            .def("ApplyEditsToList", &This::_ApplyEditsToList)
            .add_property("expired", &This::_IsExpired)
            .add_static_property("invalidIndex", &This::_GetInvalidIndex)
            .def(self == self)
            .def(self != self)
            .def(self <  self)
            .def(self <= self)
            .def(self >  self)
            .def(self >= self)
            .def(self == other<value_vector_type>())
            .def(self != other<value_vector_type>())
            .def(self <  other<value_vector_type>())
            .def(self <= other<value_vector_type>())
            .def(self >  other<value_vector_type>())
            .def(self >= other<value_vector_type>())
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

    static pxr_boost::python::list _GetItemSlice(const Type& x,
                                             const pxr_boost::python::slice& index)
    {
        using namespace pxr_boost::python;

        list result;

        if (x._Validate()) {
            try {
                slice::range<typename Type::const_iterator> range =
                    index.get_indicies(x.begin(), x.end());
                for (; range.start != range.stop; range.start += range.step) {
                    result.append(*range.start);
                }
                result.append(*range.start);
            }
            catch (const std::invalid_argument&) {
                // Ignore.
            }
        }

        return result;
    }

    static void _SetItemIndex(Type& x, int index, const value_type& value)
    {
        x[TfPyNormalizeIndex(index, x._GetSize(), true)] = value;
    }

    static void _SetItemSlice(Type& x, const pxr_boost::python::slice& index,
                              const value_vector_type& values)
    {
        using namespace pxr_boost::python;

        if (! x._Validate()) {
            return;
        }

        // Get the range and the number of items in the slice.
        size_t start, step, count;
        try {
            slice::range<typename Type::iterator> range =
                index.get_indicies(x.begin(), x.end());
            start = range.start - x.begin();
            step  = range.step;
            count = 1 + (range.stop - range.start) / range.step;
        }
        catch (const std::invalid_argument&) {
            // Empty range.
            extract<int> e(index.start());
            start = e.check() ? TfPyNormalizeIndex(e(), x._GetSize(), true) : 0;
            step  = 0;
            count = 0;
        }

        if (TfPyIsNone(index.step())) {
            // Replace contiguous sequence with values.
            x._Edit(start, count, values);
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
                x._Edit(start, count, values);
            }
            else {
                SdfChangeBlock block;
                for (size_t i = 0, j = start; i != count; j += step, ++i) {
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

    static void _DelItemSlice(Type& x, const pxr_boost::python::slice& index)
    {
        using namespace pxr_boost::python;

        if (x._Validate()) {
            try {
                // Get the range and the number of items in the slice.
                slice::range<typename Type::iterator> range =
                    index.get_indicies(x.begin(), x.end());
                size_t start = range.start - x.begin();
                size_t step  = range.step;
                size_t count = 1 + (range.stop - range.start) / range.step;

                // Erase items.
                if (step == 1) {
                    x._Edit(start, count, value_vector_type());
                }
                else {
                    SdfChangeBlock block;
                    value_vector_type empty;
                    for (size_t j = start; count > 0; j += step - 1, --count) {
                        x._Edit(j, 1, empty);
                    }
                }
            }
            catch (const std::invalid_argument&) {
                // Empty slice -- do nothing.
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
