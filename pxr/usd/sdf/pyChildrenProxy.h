//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_PY_CHILDREN_PROXY_H
#define PXR_USD_SDF_PY_CHILDREN_PROXY_H

/// \file sdf/pyChildrenProxy.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/childrenProxy.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_OPEN_SCOPE

template <class _View>
class SdfPyChildrenProxy {
public:
    typedef _View View;
    typedef SdfChildrenProxy<View> Proxy;
    typedef typename Proxy::key_type key_type;
    typedef typename Proxy::mapped_type mapped_type;
    typedef typename Proxy::mapped_vector_type mapped_vector_type;
    typedef typename Proxy::size_type size_type;
    typedef SdfPyChildrenProxy<View> This;

    SdfPyChildrenProxy(const Proxy& proxy) : _proxy(proxy)
    {
        _Init();
    }

    SdfPyChildrenProxy(const View& view, const std::string& type,
                       int permission = Proxy::CanSet |
                                        Proxy::CanInsert |
                                        Proxy::CanErase) :
        _proxy(view, type, permission)
    {
        _Init();
    }

    bool operator==(const This& other) const
    {
        return _proxy == other._proxy;
    }

    bool operator!=(const This& other) const
    {
        return _proxy != other._proxy;
    }

private:
    typedef typename Proxy::const_iterator _const_iterator;
    typedef typename View::const_iterator _view_const_iterator;

    struct _ExtractItem {
        static nanobind::object Get(const _const_iterator& i)
        {
            return nanobind::make_tuple(i->first, i->second);
        }
    };

    struct _ExtractKey {
        static nanobind::object Get(const _const_iterator& i)
        {
            return nanobind::cast(i->first);
        }
    };

    struct _ExtractValue {
        static nanobind::object Get(const _const_iterator& i)
        {
            return nanobind::cast(i->second);
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const nanobind::object& object) :
            _object(object),
            _owner(nanobind::cast<const This&>(object)._proxy)
        {
            _cur = _owner.begin();
        }

        _Iterator<E> GetCopy() const
        {
            return *this;
        }

        nanobind::object GetNext()
        {
            if (_cur == _owner.end()) {
                TfPyThrowStopIteration("End of ChildrenProxy iteration");
            }
            nanobind::object result = E::Get(_cur);
            ++_cur;
            return result;
        }

    private:
        nanobind::object _object;
        const Proxy& _owner;
        _const_iterator _cur;
    };

    void _Init()
    {
        TfPyWrapOnce<This>(&This::_Wrap);
    }

    static void _Wrap()
    {
        namespace nb = nanobind;

        std::string name = _GetName();
        nb::handle scope = TfPyGetCurrentNanobindModule();
        if (!scope.is_valid()) {
            scope = nb::borrow(PyImport_AddModule("pxr.Sdf"));
        }

        nb::class_<This> cls(scope, name.c_str());
        cls
            .def("__repr__", &This::_GetRepr, TfPyRaiseOnError())
            .def("__len__", &This::_GetSize, TfPyRaiseOnError())
            .def("__getitem__", &This::_GetItemByKey, TfPyRaiseOnError())
            .def("__getitem__", &This::_GetItemByIndex, TfPyRaiseOnError())
            .def("__setitem__", &This::_SetItemByKey, TfPyRaiseOnError())
            .def("__setitem__", &This::_SetItemBySlice, TfPyRaiseOnError())
            .def("__delitem__", &This::_DelItemByKey, TfPyRaiseOnError())
            .def("__delitem__", &This::_DelItemByIndex, TfPyRaiseOnError())
            .def("__contains__", &This::_HasKey, TfPyRaiseOnError())
            .def("__contains__", &This::_HasValue, TfPyRaiseOnError())
            .def("__iter__",   &This::_GetValueIterator, TfPyRaiseOnError())
            .def("clear", &This::_Clear, TfPyRaiseOnError())
            .def("append", &This::_AppendItem, TfPyRaiseOnError())
            .def("insert", &This::_InsertItemByIndex, TfPyRaiseOnError())
            .def("get", &This::_PyGet, TfPyRaiseOnError())
            .def("get", &This::_PyGetDefault, TfPyRaiseOnError())
            .def("items", &This::_GetItemIterator, TfPyRaiseOnError())
            .def("keys", &This::_GetKeyIterator, TfPyRaiseOnError())
            .def("values", &This::_GetValueIterator, TfPyRaiseOnError())
            .def("index", &This::_FindIndexByKey, TfPyRaiseOnError())
            .def("index", &This::_FindIndexByValue, TfPyRaiseOnError())
            .def(nb::self == nb::self, TfPyRaiseOnError())
            .def(nb::self != nb::self, TfPyRaiseOnError())
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
        std::string name = "ChildrenProxy_" +
                           ArchGetDemangled<View>();
        name = TfStringReplace(name, " ", "_");
        name = TfStringReplace(name, ",", "_");
        name = TfStringReplace(name, "::", "_");
        name = TfStringReplace(name, "<", "_");
        name = TfStringReplace(name, ">", "_");
        return name;
    }

    const View& _GetView() const
    {
        return _proxy._view;
    }

    View& _GetView()
    {
        return _proxy._view;
    }

    std::string _GetRepr() const
    {
        std::string result("{");
        if (! _proxy.empty()) {
            _const_iterator i = _proxy.begin(), n = _proxy.end();
            result += TfPyRepr(i->first) + ": " + TfPyRepr(i->second);
            while (++i != n) {
                result += ", " + TfPyRepr(i->first) +
                          ": " + TfPyRepr(i->second);
            }
        }
        result += "}";
        return result;
    }

    size_type _GetSize() const
    {
        return _proxy.size();
    }

    mapped_type _GetItemByKey(const key_type& key) const
    {
        _view_const_iterator i = _GetView().find(key);
        if (i == _GetView().end()) {
            TfPyThrowIndexError(TfPyRepr(key));
            return mapped_type();
        }
        else {
            return *i;
        }
    }

    mapped_type _GetItemByIndex(int index) const
    {
        index = TfPyNormalizeIndex(index, _proxy.size(), true /*throwError*/);
        return _GetView()[index];
    }

    void _SetItemByKey(const key_type& key, const mapped_type& value)
    {
        TF_CODING_ERROR("can't directly reparent a %s",
                        _proxy._GetType().c_str());
    }

    void _SetItemBySlice(const nanobind::slice& slice,
                         const mapped_vector_type& values)
    {
        if (! slice.attr("start").is_none() ||
            ! slice.attr("stop").is_none() ||
            ! slice.attr("step").is_none()) {
            TfPyThrowIndexError("can only assign to full slice [:]");
        }
        else {
            _proxy._Copy(values);
        }
    }

    void _DelItemByKey(const key_type& key)
    {
        if (_GetView().find(key) == _GetView().end()) {
            TfPyThrowIndexError(TfPyRepr(key));
        }
        _proxy._Erase(key);
    }

    void _DelItemByIndex(int index)
    {
        _proxy._Erase(_GetView().key(_GetItemByIndex(index)));
    }

    void _Clear()
    {
        _proxy._Copy(mapped_vector_type());
    }

    void _AppendItem(const mapped_type& value)
    {
        _proxy._Insert(value, _proxy.size());
    }

    void _InsertItemByIndex(int index, const mapped_type& value)
    {
        // Note that -1 below means to insert at end for the _proxy._Insert API.
        index = index < (int)_proxy.size() 
            ? TfPyNormalizeIndex(index, _proxy.size(), false /*throwError*/)
            : -1;

        _proxy._Insert(value, index);
    }

    nanobind::object _PyGet(const key_type& key) const
    {
        _view_const_iterator i = _GetView().find(key);
        return i == _GetView().end() ? nanobind::none() :
                                       nanobind::cast(*i);
    }

    nanobind::object _PyGetDefault(const key_type& key,
                                   const mapped_type& def) const
    {
        _view_const_iterator i = _GetView().find(key);
        return i == _GetView().end() ? nanobind::cast(def) :
                                       nanobind::cast(*i);
    }

    bool _HasKey(const key_type& key) const
    {
        return _GetView().find(key) != _GetView().end();
    }

    bool _HasValue(const mapped_type& value) const
    {
        return _GetView().find(value) != _GetView().end();
    }

    static
    _Iterator<_ExtractItem> _GetItemIterator(const nanobind::object &x)
    {
        return _Iterator<_ExtractItem>(x);
    }

    static
    _Iterator<_ExtractKey> _GetKeyIterator(const nanobind::object &x)
    {
        return _Iterator<_ExtractKey>(x);
    }

    static
    _Iterator<_ExtractValue> _GetValueIterator(const nanobind::object &x)
    {
        return _Iterator<_ExtractValue>(x);
    }

    template <class E>
    nanobind::list _Get() const
    {
        nanobind::list result;
        for (_const_iterator i = _proxy.begin(), n = _proxy.end(); i != n; ++i){
            result.append(E::Get(i));
        }
        return result;
    }

    int _FindIndexByKey(const key_type& key) const
    {
        size_t i = std::distance(_GetView().begin(), _GetView().find(key));
        return i == _GetView().size() ? -1 : i;
    }

    int _FindIndexByValue(const mapped_type& value) const
    {
        size_t i = std::distance(_GetView().begin(), _GetView().find(value));
        return i == _GetView().size() ? -1 : i;
    }

private:
    Proxy _proxy;

    template <class E> friend class _Iterator;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_PY_CHILDREN_PROXY_H
