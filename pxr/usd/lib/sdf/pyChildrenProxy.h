//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
/// \file sdf/pyChildrenProxy.h

#ifndef SDF_PYCHILDRENPROXY_H
#define SDF_PYCHILDRENPROXY_H

#include <boost/python.hpp>
#include <boost/python/slice.hpp>

#include "pxr/usd/sdf/childrenProxy.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"

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
        static boost::python::object Get(const _const_iterator& i)
        {
            return boost::python::make_tuple(i->first, i->second);
        }
    };

    struct _ExtractKey {
        static boost::python::object Get(const _const_iterator& i)
        {
            return boost::python::object(i->first);
        }
    };

    struct _ExtractValue {
        static boost::python::object Get(const _const_iterator& i)
        {
            return boost::python::object(i->second);
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const boost::python::object& object) :
            _object(object),
            _owner(boost::python::extract<const This&>(object)()._proxy)
        {
            _cur = _owner.begin();
        }

        _Iterator<E> GetCopy() const
        {
            return *this;
        }

        boost::python::object GetNext()
        {
            if (_cur == _owner.end()) {
                TfPyThrowStopIteration("End of ChildrenProxy iteration");
            }
            boost::python::object result = E::Get(_cur);
            ++_cur;
            return result;
        }

    private:
        boost::python::object _object;
        const Proxy& _owner;
        _const_iterator _cur;
    };

    void _Init()
    {
        TfPyWrapOnce<This>(&This::_Wrap);
    }

    static void _Wrap()
    {
        using namespace boost::python;

        std::string name = _GetName();

        scope thisScope =
        class_<This>(name.c_str(), no_init)
            .def("__repr__", &This::_GetRepr, TfPyRaiseOnError<>())
            .def("__len__", &This::_GetSize, TfPyRaiseOnError<>())
            .def("__getitem__", &This::_GetItemByKey, TfPyRaiseOnError<>())
            .def("__getitem__", &This::_GetItemByIndex, TfPyRaiseOnError<>())
            .def("__setitem__", &This::_SetItemByKey, TfPyRaiseOnError<>())
            .def("__setitem__", &This::_SetItemBySlice, TfPyRaiseOnError<>())
            .def("__delitem__", &This::_DelItemByKey, TfPyRaiseOnError<>())
            .def("__delitem__", &This::_DelItemByIndex, TfPyRaiseOnError<>())
            .def("__contains__", &This::_HasKey, TfPyRaiseOnError<>())
            .def("__contains__", &This::_HasValue, TfPyRaiseOnError<>())
            .def("__iter__",   &This::_GetValueIterator, TfPyRaiseOnError<>())
            .def("itervalues", &This::_GetValueIterator, TfPyRaiseOnError<>())
            .def("iterkeys",   &This::_GetKeyIterator, TfPyRaiseOnError<>())
            .def("iteritems",  &This::_GetItemIterator, TfPyRaiseOnError<>())
            .def("clear", &This::_Clear, TfPyRaiseOnError<>())
            .def("append", &This::_AppendItem, TfPyRaiseOnError<>())
            .def("insert", &This::_InsertItemByIndex, TfPyRaiseOnError<>())
            .def("get", &This::_PyGet, TfPyRaiseOnError<>())
            .def("get", &This::_PyGetDefault, TfPyRaiseOnError<>())
            .def("has_key", &This::_HasKey, TfPyRaiseOnError<>())
            .def("items", &This::_GetItems, TfPyRaiseOnError<>())
            .def("keys", &This::_GetKeys, TfPyRaiseOnError<>())
            .def("values", &This::_GetValues, TfPyRaiseOnError<>())
            .def("index", &This::_FindIndexByKey, TfPyRaiseOnError<>())
            .def("index", &This::_FindIndexByValue, TfPyRaiseOnError<>())
            .def("__eq__", &This::operator==, TfPyRaiseOnError<>())
            .def("__ne__", &This::operator!=, TfPyRaiseOnError<>())
            ;

        class_<_Iterator<_ExtractItem> >
            ((name + "_Iterator").c_str(), no_init)
            .def("__iter__", &This::template _Iterator<_ExtractItem>::GetCopy)
            .def("next", &This::template _Iterator<_ExtractItem>::GetNext)
            ;

        class_<_Iterator<_ExtractKey> >
            ((name + "_KeyIterator").c_str(), no_init)
            .def("__iter__", &This::template _Iterator<_ExtractKey>::GetCopy)
            .def("next", &This::template _Iterator<_ExtractKey>::GetNext)
            ;

        class_<_Iterator<_ExtractValue> >
            ((name + "_ValueIterator").c_str(), no_init)
            .def("__iter__", &This::template _Iterator<_ExtractValue>::GetCopy)
            .def("next", &This::template _Iterator<_ExtractValue>::GetNext)
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
        if (not _proxy.empty()) {
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
        if (index >= _proxy.size()) {
            TfPyThrowIndexError("list index out of range");
        }
        return _GetView()[index];
    }

    void _SetItemByKey(const key_type& key, const mapped_type& value)
    {
        TF_CODING_ERROR("can't directly reparent a %s",
                        _proxy._GetType().c_str());
    }

    void _SetItemBySlice(const boost::python::slice& slice,
                         const mapped_vector_type& values)
    {
        if (not TfPyIsNone(slice.start()) or
            not TfPyIsNone(slice.stop()) or
            not TfPyIsNone(slice.step())) {
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
        if (index < -1 or index > static_cast<int>(_proxy.size())) {
            TfPyThrowIndexError("list index out of range");
        }
        _proxy._Insert(value, index);
    }

    boost::python::object _PyGet(const key_type& key) const
    {
        _view_const_iterator i = _GetView().find(key);
        return i == _GetView().end() ? boost::python::object() :
                                       boost::python::object(*i);
    }

    boost::python::object _PyGetDefault(const key_type& key,
                                        const mapped_type& def) const
    {
        _view_const_iterator i = _GetView().find(key);
        return i == _GetView().end() ? boost::python::object(def) :
                                       boost::python::object(*i);
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
    _Iterator<_ExtractItem> _GetItemIterator(const boost::python::object &x)
    {
        return _Iterator<_ExtractItem>(x);
    }

    static
    _Iterator<_ExtractKey> _GetKeyIterator(const boost::python::object &x)
    {
        return _Iterator<_ExtractKey>(x);
    }

    static
    _Iterator<_ExtractValue> _GetValueIterator(const boost::python::object &x)
    {
        return _Iterator<_ExtractValue>(x);
    }

    template <class E>
    boost::python::list _Get() const
    {
        boost::python::list result;
        for (_const_iterator i = _proxy.begin(), n = _proxy.end(); i != n; ++i){
            result.append(E::Get(i));
        }
        return result;
    }

    boost::python::list _GetItems() const
    {
        return _Get<_ExtractItem>();
    }

    boost::python::list _GetKeys() const
    {
        return _Get<_ExtractKey>();
    }

    boost::python::list _GetValues() const
    {
        return _Get<_ExtractValue>();
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

#endif
