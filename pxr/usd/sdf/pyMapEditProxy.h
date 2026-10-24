//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PY_MAP_EDIT_PROXY_H
#define PXR_USD_SDF_PY_MAP_EDIT_PROXY_H

/// \file sdf/pyMapEditProxy.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_OPEN_SCOPE

template <class T>
class SdfPyWrapMapEditProxy {
public:
    typedef T Type;
    typedef typename Type::key_type key_type;
    typedef typename Type::mapped_type mapped_type;
    typedef typename Type::value_type value_type;
    typedef typename Type::iterator iterator;
    typedef typename Type::const_iterator const_iterator;
    typedef SdfPyWrapMapEditProxy<Type> This;

    SdfPyWrapMapEditProxy()
    {
        TfPyWrapOnce<Type>(&This::_Wrap);
    }

private:
    typedef std::pair<key_type, mapped_type> pair_type;

    struct _ExtractItem {
        static nanobind::object Get(const const_iterator& i)
        {
            return nanobind::make_tuple(i->first, i->second);
        }
    };

    struct _ExtractKey {
        static nanobind::object Get(const const_iterator& i)
        {
            return nanobind::cast(i->first);
        }
    };

    struct _ExtractValue {
        static nanobind::object Get(const const_iterator& i)
        {
            return nanobind::cast(i->second);
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const nanobind::object& object) :
            _object(object),
            _owner(nanobind::cast<const Type&>(object)),
            _cur(_owner.begin()),
            _end(_owner.end())
        {
            // Do nothing
        }

        _Iterator<E> GetCopy() const
        {
            return *this;
        }

        nanobind::object GetNext()
        {
            if (_cur == _end) {
                TfPyThrowStopIteration("End of MapEditProxy iteration");
            }
            nanobind::object result = E::Get(_cur);
            ++_cur;
            return result;
        }

    private:
        nanobind::object _object;
        const Type& _owner;
        const_iterator _cur;
        const_iterator _end;
    };

    static void _Wrap()
    {
        namespace nb = nanobind;

        std::string name = _GetName();
        nb::handle scope = TfPyGetCurrentNanobindModule();
        if (!scope.is_valid()) {
            scope = nb::borrow(PyImport_AddModule("pxr.Sdf"));
        }

        nb::class_<Type> cls(scope, name.c_str());
        cls
            .def(nb::init<>())
            .def("__repr__", &This::_GetRepr)
            .def("__str__", &This::_GetStr)
            .def("__len__", &Type::size)
            .def("__getitem__", &This::_GetItem)
            .def("__setitem__", &This::_SetItem)
            .def("__delitem__", &This::_DelItem)
            .def("__contains__", &This::_HasKey)
            .def("__iter__",   &This::_GetKeyIterator)
            .def("values", &This::_GetValueIterator)
            .def("keys",   &This::_GetKeyIterator)
            .def("items",  &This::_GetItemIterator)
            .def("clear", &Type::clear)
            .def("get", &This::_PyGet)
            .def("get", &This::_PyGetDefault)
            .def("pop", &This::_Pop)
            .def("popitem", &This::_PopItem)
            .def("setdefault", &This::_SetDefault)
            .def("update", &This::_UpdateDict)
            .def("update", &This::_UpdateList)
            .def("copy", &This::_Copy)
            .def_prop_ro("expired", &Type::IsExpired)
            .def("__bool__", &This::_IsValid)
            .def(nb::self == nb::self)
            .def(nb::self != nb::self)
            ;

        nb::class_<_Iterator<_ExtractItem> >(
            cls, (name + "_Iterator").c_str())
            .def("__iter__", &This::template _Iterator<_ExtractItem>::GetCopy)
            .def("__next__", &This::template _Iterator<_ExtractItem>::GetNext)
            ;

        nb::class_<_Iterator<_ExtractKey> >(
            cls, (name + "_KeyIterator").c_str())
            .def("__iter__", &This::template _Iterator<_ExtractKey>::GetCopy)
            .def("__next__", &This::template _Iterator<_ExtractKey>::GetNext)
            ;

        nb::class_<_Iterator<_ExtractValue> >(
            cls, (name + "_ValueIterator").c_str())
            .def("__iter__", &This::template _Iterator<_ExtractValue>::GetCopy)
            .def("__next__", &This::template _Iterator<_ExtractValue>::GetNext)
            ;
    }

    static std::string _GetName()
    {
        std::string name = "MapEditProxy_" +
                           ArchGetDemangled<typename Type::Type>();
        name = TfStringReplace(name, " ", "_");
        name = TfStringReplace(name, ",", "_");
        name = TfStringReplace(name, "::", "_");
        name = TfStringReplace(name, "<", "_");
        name = TfStringReplace(name, ">", "_");
        return name;
    }

    static std::string _GetRepr(const Type& x)
    {
        std::string arg;
        if (x) {
            arg = TfStringPrintf("<%s>", x._Location().c_str());
        }
        else {
            arg = "<invalid>";
        }
        return TF_PY_REPR_PREFIX + _GetName() + "(" + arg + ")";
    }

    static std::string _GetStr(const Type& x)
    {
        std::string result("{");
        if (x && ! x.empty()) {
            const_iterator i = x.begin(), n = x.end();
            result += TfPyRepr(i->first) + ": " + TfPyRepr(i->second);
            while (++i != n) {
                result +=", " + TfPyRepr(i->first) + ": " + TfPyRepr(i->second);
            }
        }
        result += "}";
        return result;
    }

    static mapped_type _GetItem(const Type& x, const key_type& key)
    {
        const_iterator i = x.find(key);
        if (i == x.end()) {
            TfPyThrowKeyError(TfPyRepr(key));
            return mapped_type();
        }
        else {
            return i->second;
        }
    }

    static void _SetItem(Type& x, const key_type& key, const mapped_type& value)
    {
        std::pair<typename Type::iterator, bool> i =
            x.insert(value_type(key, value));
        if (! i.second && i.first != typename Type::iterator()) {
            i.first->second = value;
        }
    }

    static void _DelItem(Type& x, const key_type& key)
    {
        x.erase(key);
    }

    static bool _HasKey(const Type& x, const key_type& key)
    {
        return x.count(key) != 0;
    }

    static _Iterator<_ExtractItem> 
    _GetItemIterator(const nanobind::object& x)
    {
        return _Iterator<_ExtractItem>(x);
    }

    static _Iterator<_ExtractKey> 
    _GetKeyIterator(const nanobind::object& x)
    {
        return _Iterator<_ExtractKey>(x);
    }

    static _Iterator<_ExtractValue> 
    _GetValueIterator(const nanobind::object& x)
    {
        return _Iterator<_ExtractValue>(x);
    }

    static nanobind::object _PyGet(const Type& x, const key_type& key)
    {
        const_iterator i = x.find(key);
        return i == x.end() ? nanobind::none() :
                              nanobind::cast(i->second);
    }

    static mapped_type _PyGetDefault(const Type& x, const key_type& key,
                                     const mapped_type& def)
    {
        const_iterator i = x.find(key);
        return i == x.end() ? def : i->second;
    }

    template <class E>
    static nanobind::list _Get(const Type& x)
    {
        nanobind::list result;
        for (const_iterator i = x.begin(), n = x.end(); i != n; ++i) {
            result.append(E::Get(i));
        }
        return result;
    }

    static mapped_type _Pop(Type& x, const key_type& key)
    {
        iterator i = x.find(key);
        if (i == x.end()) {
            TfPyThrowKeyError(TfPyRepr(key));
            return mapped_type();
        }
        else {
            mapped_type result = i->second;
            x.erase(i);
            return result;
        }
    }

    static nanobind::tuple _PopItem(Type& x)
    {
        if (x.empty()) {
            TfPyThrowKeyError("MapEditProxy is empty");
            return nanobind::tuple();
        }
        else {
            iterator i = x.begin();
            value_type result = *i;
            x.erase(i);
            return nanobind::make_tuple(result.first, result.second);
        }
    }

    static mapped_type _SetDefault(Type& x, const key_type& key,
                                   const mapped_type& def)
    {
        const_iterator i = x.find(key);
        if (i != x.end()) {
            return i->second;
        }
        else {
            return x[key] = def;
        }
    }

    static void _Update(Type& x, const std::vector<pair_type>& values)
    {
        SdfChangeBlock block;
        TF_FOR_ALL(i, values) {
            x[i->first] = i->second;
        }
    }

    static void _UpdateDict(Type& x, const nanobind::dict& d)
    {
        std::vector<pair_type> values;
        for (auto item : d) {
            values.push_back(pair_type(
                nanobind::cast<key_type>(item.first),
                nanobind::cast<mapped_type>(item.second)));
        }
        _Update(x, values);
    }

    static void _UpdateList(Type& x, const nanobind::list& pairs)
    {
        std::vector<pair_type> values;
        for (size_t i = 0, n = pairs.size(); i != n; ++i) {
            values.push_back(pair_type(
                nanobind::cast<key_type>(pairs[i][0]),
                nanobind::cast<mapped_type>(pairs[i][1])));
        }
        _Update(x, values);
    }

    static void _Copy(Type& x, const typename Type::Type& other)
    {
        x = other;
    }

    static bool _IsValid(const Type& x)
    {
        return static_cast<bool>(x);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PY_MAP_EDIT_PROXY_H
