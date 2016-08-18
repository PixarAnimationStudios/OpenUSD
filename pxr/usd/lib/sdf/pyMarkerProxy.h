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

/// \file sdf/pyMarkerProxy.h

#include <boost/python.hpp>

#include "pxr/usd/sdf/changeBlock.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <deque>
#include <map>

template <class SpecType>
class Sdf_PyMarkerPolicy {
public:
    static SdfPathVector GetMarkerPaths(const SdfHandle<SpecType>& spec);
    static std::string GetMarker(const SdfHandle<SpecType>& spec,
                                 const SdfPath& path);
    static void SetMarker(const SdfHandle<SpecType>& spec,
                          const SdfPath& path, const std::string& marker);
    static void SetMarkers(const SdfHandle<SpecType>& spec,
                           const std::map<SdfPath, std::string>& markers);
};

template <class SpecType>
class SdfPyMarkerProxy {
public:
    typedef SdfPath key_type;
    typedef std::string mapped_type;
    typedef std::pair<key_type, mapped_type> value_type;

    typedef SdfPyMarkerProxy<SpecType> This;
    typedef SdfHandle<SpecType> OwnerSpecHandle;
    typedef Sdf_PyMarkerPolicy<SpecType> MarkerPolicy;

    SdfPyMarkerProxy(const OwnerSpecHandle& spec)
        : _spec(spec)
    {
        TfPyWrapOnce<This>(&This::_Wrap);
    }

    bool operator==(const This& other) const
    {
        return _spec == other._spec;
    }

    bool operator!=(const This& other) const
    {
        return _spec != other._spec;
    }

private:
    struct _ExtractItem {
        static boost::python::object Get(const OwnerSpecHandle& spec,
                                         const SdfPath& markerPath)
        {
            return boost::python::make_tuple(
                markerPath, MarkerPolicy::GetMarker(spec, markerPath));
        }
    };

    struct _ExtractKey {
        static boost::python::object Get(const OwnerSpecHandle& spec,
                                         const SdfPath& markerPath)
        {
            return boost::python::object(markerPath);
        }
    };

    struct _ExtractValue {
        static boost::python::object Get(const OwnerSpecHandle& spec,
                                         const SdfPath& markerPath)
        {
            return boost::python::object(
                MarkerPolicy::GetMarker(spec, markerPath));
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const OwnerSpecHandle& spec)
            : _spec(spec)
        {
            if (_spec) {
                const SdfPathVector paths = MarkerPolicy::GetMarkerPaths(_spec);
                _markerPaths.insert(
                    _markerPaths.end(), paths.begin(), paths.end());
            }
        }

        _Iterator<E> GetCopy() const
        {
            return *this;
        }

        boost::python::object GetNext()
        {
            if (_markerPaths.empty()) {
                TfPyThrowStopIteration("End of MarkerProxy iteration");
            }

            boost::python::object result = E::Get(_spec, _markerPaths.front());
            _markerPaths.pop_front();
            return result;
        }

    private:
        OwnerSpecHandle _spec;
        std::deque<SdfPath> _markerPaths;
    };

    std::string _GetStr() const
    {
        std::string result("{");

        if (_Validate()) {
            const SdfPathVector markerPaths=MarkerPolicy::GetMarkerPaths(_spec);
            for (int i = 0; i < markerPaths.size(); ++i) {
                if (i != 0) {
                    result += ", ";
                }

                const std::string marker = 
                    MarkerPolicy::GetMarker(_spec, markerPaths[i]);

                result += (TfPyRepr(markerPaths[i]) + ": " + TfPyRepr(marker));
            }
        }

        result += "}";
        return result;
    }

    static std::string _GetName()
    {
        std::string name = "MarkerProxy_" +
                           ArchGetDemangled<SpecType>();
        name = TfStringReplace(name, " ", "_");
        name = TfStringReplace(name, ",", "_");
        name = TfStringReplace(name, "::", "_");
        name = TfStringReplace(name, "<", "_");
        name = TfStringReplace(name, ">", "_");
        return name;
    }

    size_t _GetSize() const
    {
        return _Validate() ? MarkerPolicy::GetMarkerPaths(_spec).size() : 0;
    }

    mapped_type _GetItem(const key_type& key)
    {
        if (not _Validate()) {
            return mapped_type();
        }

        const std::string marker = MarkerPolicy::GetMarker(_spec, key);
        if (marker.empty()) {
            TfPyThrowKeyError(TfPyRepr(key));
            return mapped_type();
        }
        else {
            return marker;
        }
    }

    void _SetItem(const key_type& key, const mapped_type& value)
    {
        if (not _Validate()) {
            return;
        }

        MarkerPolicy::SetMarker(_spec, key, value);
    }

    void _DelItem(const key_type& key)
    {
        if (not _Validate()) {
            return;
        }

        MarkerPolicy::SetMarker(_spec, key, std::string());
    }

    void _Clear()
    {
        if (not _Validate()) {
            return;
        }

        SdfChangeBlock block;
        const SdfPathVector paths = MarkerPolicy::GetMarkerPaths(_spec);
        TF_FOR_ALL(path, paths) {
            MarkerPolicy::SetMarker(_spec, *path, std::string());
        }
    }

    bool _HasKey(const key_type& key)
    {
        return _Validate() ? 
            not MarkerPolicy::GetMarker(_spec, key).empty() : false;
    }    

    _Iterator<_ExtractItem> _GetItemIterator()
    {
        _Validate();
        return _Iterator<_ExtractItem>(_spec);
    }

    _Iterator<_ExtractKey> _GetKeyIterator()
    {
        _Validate();
        return _Iterator<_ExtractKey>(_spec);
    }

    _Iterator<_ExtractValue> _GetValueIterator()
    {
        _Validate();
        return _Iterator<_ExtractValue>(_spec);
    }

    boost::python::object _PyGet(const key_type& key)
    {
        if (not _Validate()) {
            return boost::python::object();
        }

        const std::string marker = MarkerPolicy::GetMarker(_spec, key);
        return marker.empty() ? boost::python::object() :
                                boost::python::object(marker);
    }

    mapped_type _PyGetDefault(const key_type& key, const mapped_type& def)
    {
        if (not _Validate()) {
            return mapped_type();
        }

        const std::string marker = MarkerPolicy::GetMarker(_spec, key);
        return marker.empty() ? def : marker;
    }

    template <class E>
    boost::python::list Get()
    {
        boost::python::list result;

        if (_Validate()) {
            const SdfPathVector markerPaths=MarkerPolicy::GetMarkerPaths(_spec);
            TF_FOR_ALL(path, markerPaths) {
                result.append(E::Get(_spec, *path));
            }
        }

        return result;
    }

    boost::python::list _GetItems()
    {
        return Get<_ExtractItem>();
    }

    boost::python::list _GetKeys()
    {
        return Get<_ExtractKey>();
    }

    boost::python::list _GetValues()
    {
        return Get<_ExtractValue>();
    }

    mapped_type _Pop(const key_type& key)
    {
        if (not _Validate()) {
            return mapped_type();
        }

        const std::string marker = MarkerPolicy::GetMarker(_spec, key);
        if (marker.empty()) {
            TfPyThrowKeyError(TfPyRepr(key));
            return mapped_type();
        }
        else {
            MarkerPolicy::SetMarker(_spec, key, std::string());
            return marker;
        }
    }

    boost::python::tuple _PopItem()
    {
        if (not _Validate()) {
            return boost::python::tuple();
        }

        const SdfPathVector markerPaths = MarkerPolicy::GetMarkerPaths(_spec);
        if (markerPaths.empty()) {
            TfPyThrowKeyError("Marker proxy is empty");
            return boost::python::tuple();
        }
        else {
            const std::string marker = 
                MarkerPolicy::GetMarker(_spec, markerPaths.front());
            MarkerPolicy::SetMarker(_spec, markerPaths.front(), std::string());

            return boost::python::make_tuple(markerPaths.front(), marker);
        }
    }

    mapped_type SetDefault(const key_type& key, const mapped_type& def)
    {
        if (not _Validate()) {
            return mapped_type();
        }

        const std::string marker = MarkerPolicy::GetMarker(_spec, key);
        if (not marker.empty()) {
            return marker;
        }
        else {
            SdfChangeBlock block;
            MarkerPolicy::SetMarker(_spec, key, def);
            return def;
        }
    }

    void _Update(const std::vector<value_type>& values)
    {
        if (not _Validate()) {
            return;
        }

        SdfChangeBlock block;
        TF_FOR_ALL(i, values) {
            MarkerPolicy::SetMarker(_spec, i->first, i->second);
        }
    }

    void _UpdateDict(const boost::python::dict& d)
    {
        _UpdateList(d.items());
    }

    void _UpdateList(const boost::python::list& pairs)
    {
        using namespace boost::python;

        std::vector<value_type> values;
        for (int i = 0, n = len(pairs); i != n; ++i) {
            values.push_back(value_type(
                extract<key_type>(pairs[i][0]),
                extract<mapped_type>(pairs[i][1])));
        }
        _Update(values);
    }

    void Copy(const boost::python::dict& other)
    {
        if (not _Validate()) {
            return;
        }

        std::map<SdfPath, std::string> markerMap;

        boost::python::list keys = other.keys();
        size_t numKeys = len(other);
        for (size_t i = 0; i != numKeys; i++) {
            key_type key    = boost::python::extract<key_type>(keys[i]);
            mapped_type val = boost::python::extract<mapped_type>(other[keys[i]]);
            markerMap[key] = val;
        }

        MarkerPolicy::SetMarkers(_spec, markerMap);
    }

    bool _NonZero()
    {
        return _spec;
    }

    bool _Validate() const
    {
        if (not _spec) {
            TF_CODING_ERROR("Accessing an expired attribute");
            return false;
        }
        return true;
    }

    static void _Wrap()
    {
        using namespace boost::python;

        const std::string name = _GetName();

        scope s = 
            class_<This>(name.c_str(), no_init)
            .def("__str__", &This::_GetStr, TfPyRaiseOnError<>())
            .def("__len__", &This::_GetSize, TfPyRaiseOnError<>())
            .def("__getitem__", &This::_GetItem, TfPyRaiseOnError<>())
            .def("__setitem__", &This::_SetItem, TfPyRaiseOnError<>())
            .def("__delitem__", &This::_DelItem, TfPyRaiseOnError<>())
            .def("__contains__", &This::_HasKey, TfPyRaiseOnError<>())
            .def("__iter__",   &This::_GetKeyIterator, TfPyRaiseOnError<>())
            .def("itervalues", &This::_GetValueIterator, TfPyRaiseOnError<>())
            .def("iterkeys",   &This::_GetKeyIterator, TfPyRaiseOnError<>())
            .def("iteritems",  &This::_GetItemIterator, TfPyRaiseOnError<>())
            .def("clear", &This::_Clear, TfPyRaiseOnError<>())
            .def("get", &This::_PyGet, TfPyRaiseOnError<>())
            .def("get", &This::_PyGetDefault, TfPyRaiseOnError<>())
            .def("has_key", &This::_HasKey, TfPyRaiseOnError<>())
            .def("items", &This::_GetItems, TfPyRaiseOnError<>())
            .def("keys", &This::_GetKeys, TfPyRaiseOnError<>())
            .def("values", &This::_GetValues, TfPyRaiseOnError<>())
            .def("pop", &This::_Pop, TfPyRaiseOnError<>())
            .def("popitem", &This::_PopItem, TfPyRaiseOnError<>())
            .def("setdefault", &This::SetDefault, TfPyRaiseOnError<>())
            .def("update", &This::_UpdateDict, TfPyRaiseOnError<>())
            .def("update", &This::_UpdateList, TfPyRaiseOnError<>())
            .def("copy", &This::Copy, TfPyRaiseOnError<>())
            .def("__nonzero__", &This::_NonZero, TfPyRaiseOnError<>())
            .def(self == self)
            .def(self != self)
            ;

        class_<_Iterator<_ExtractItem> >
            ((name + "_Iterator").c_str(), no_init)
            .def("__iter__", &_Iterator<_ExtractItem>::GetCopy)
            .def("next", &_Iterator<_ExtractItem>::GetNext)
            ;

        class_<_Iterator<_ExtractKey> >
            ((name + "_KeyIterator").c_str(), no_init)
            .def("__iter__", &_Iterator<_ExtractKey>::GetCopy)
            .def("next", &_Iterator<_ExtractKey>::GetNext)
            ;

        class_<_Iterator<_ExtractValue> >
            ((name + "_ValueIterator").c_str(), no_init)
            .def("__iter__", &_Iterator<_ExtractValue>::GetCopy)
            .def("next", &_Iterator<_ExtractValue>::GetNext)
            ;
    }

private:
    OwnerSpecHandle _spec;
};

