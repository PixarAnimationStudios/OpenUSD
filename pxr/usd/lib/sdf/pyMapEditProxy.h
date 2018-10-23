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
#ifndef SDF_PYMAPEDITPROXY_H
#define SDF_PYMAPEDITPROXY_H

/// \file sdf/pyMapEditProxy.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include <boost/python.hpp>

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
        static boost::python::object Get(const const_iterator& i)
        {
            return boost::python::make_tuple(i->first, i->second);
        }
    };

    struct _ExtractKey {
        static boost::python::object Get(const const_iterator& i)
        {
            return boost::python::object(i->first);
        }
    };

    struct _ExtractValue {
        static boost::python::object Get(const const_iterator& i)
        {
            return boost::python::object(i->second);
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const boost::python::object& object) :
            _object(object),
            _owner(boost::python::extract<const Type&>(object)),
            _cur(_owner.begin()),
            _end(_owner.end())
        {
            // Do nothing
        }

        _Iterator<E> GetCopy() const
        {
            return *this;
        }

        boost::python::object GetNext()
        {
            if (_cur == _end) {
                TfPyThrowStopIteration("End of MapEditProxy iteration");
            }
            boost::python::object result = E::Get(_cur);
            ++_cur;
            return result;
        }

    private:
        boost::python::object _object;
        const Type& _owner;
        const_iterator _cur;
        const_iterator _end;
    };

    static void _Wrap()
    {
        using namespace boost::python;

        std::string name = _GetName();

        scope thisScope =
        class_<Type>(name.c_str())
            .def("__repr__", &This::_GetRepr)
            .def("__str__", &This::_GetStr)
            .def("__len__", &Type::size)
            .def("__getitem__", &This::_GetItem)
            .def("__setitem__", &This::_SetItem)
            .def("__delitem__", &This::_DelItem)
            .def("__contains__", &This::_HasKey)
            .def("__iter__",   &This::_GetKeyIterator)
            .def("itervalues", &This::_GetValueIterator)
            .def("iterkeys",   &This::_GetKeyIterator)
            .def("iteritems",  &This::_GetItemIterator)
            .def("clear", &Type::clear)
            .def("get", &This::_PyGet)
            .def("get", &This::_PyGetDefault)
            .def("has_key", &This::_HasKey)
            .def("items", &This::_GetItems)
            .def("keys", &This::_GetKeys)
            .def("values", &This::_GetValues)
            .def("pop", &This::_Pop)
            .def("popitem", &This::_PopItem)
            .def("setdefault", &This::_SetDefault)
            .def("update", &This::_UpdateDict)
            .def("update", &This::_UpdateList)
            .def("copy", &This::_Copy)
            .add_property("expired", &Type::IsExpired)
            .def("__nonzero__", &This::_NonZero)
            .def(self == self)
            .def(self != self)
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
    _GetItemIterator(const boost::python::object& x)
    {
        return _Iterator<_ExtractItem>(x);
    }

    static _Iterator<_ExtractKey> 
    _GetKeyIterator(const boost::python::object& x)
    {
        return _Iterator<_ExtractKey>(x);
    }

    static _Iterator<_ExtractValue> 
    _GetValueIterator(const boost::python::object& x)
    {
        return _Iterator<_ExtractValue>(x);
    }

    static boost::python::object _PyGet(const Type& x, const key_type& key)
    {
        const_iterator i = x.find(key);
        return i == x.end() ? boost::python::object() :
                              boost::python::object(i->second);
    }

    static mapped_type _PyGetDefault(const Type& x, const key_type& key,
                                     const mapped_type& def)
    {
        const_iterator i = x.find(key);
        return i == x.end() ? def : i->second;
    }

    template <class E>
    static boost::python::list _Get(const Type& x)
    {
        boost::python::list result;
        for (const_iterator i = x.begin(), n = x.end(); i != n; ++i) {
            result.append(E::Get(i));
        }
        return result;
    }

    static boost::python::list _GetItems(const Type& x)
    {
        return _Get<_ExtractItem>(x);
    }

    static boost::python::list _GetKeys(const Type& x)
    {
        return _Get<_ExtractKey>(x);
    }

    static boost::python::list _GetValues(const Type& x)
    {
        return _Get<_ExtractValue>(x);
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

    static boost::python::tuple _PopItem(Type& x)
    {
        if (x.empty()) {
            TfPyThrowKeyError("MapEditProxy is empty");
            return boost::python::tuple();
        }
        else {
            iterator i = x.begin();
            value_type result = *i;
            x.erase(i);
            return boost::python::make_tuple(result.first, result.second);
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

    static void _UpdateDict(Type& x, const boost::python::dict& d)
    {
        _UpdateList(x, d.items());
    }

    static void _UpdateList(Type& x, const boost::python::list& pairs)
    {
        using namespace boost::python;

        std::vector<pair_type> values;
        for (int i = 0, n = len(pairs); i != n; ++i) {
            values.push_back(pair_type(
                extract<key_type>(pairs[i][0])(),
                extract<mapped_type>(pairs[i][1])()));
        }
        _Update(x, values);
    }

    static void _Copy(Type& x, const typename Type::Type& other)
    {
        x = other;
    }

    static bool _NonZero(const Type& x)
    {
        return static_cast<bool>(x);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_PYMAPEDITPROXY_H
