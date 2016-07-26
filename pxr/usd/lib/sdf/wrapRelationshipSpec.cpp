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
/// \file wrapRelationshipSpec.cpp

#include "pxr/usd/sdf/relationshipSpec.h"
#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/pyMarkerProxy.h"
#include "pxr/usd/sdf/pySpec.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python.hpp>
#include <boost/python/slice.hpp>

using namespace boost::python;


class Sdf_RelationalAttributesProxy {
public:
    typedef std::string key_type;
    typedef SdfAttributeSpecHandle value_type;
    typedef std::vector<value_type> value_vector_type;
    typedef SdfRelationalAttributeSpecView::const_iterator const_iterator;
    typedef Sdf_RelationalAttributesProxy This;

    Sdf_RelationalAttributesProxy()
    {
        _Init();
    }

    explicit Sdf_RelationalAttributesProxy(
                const SdfRelationshipSpecHandle& rel,
                const SdfPath& key) :
        _rel(rel), _key(key),
        _view(_rel->GetAttributesForTargetPath(_key))
    {
        _Init();
    }

    bool IsValid() const
    {
        return _GetView().IsValid();
    }

    bool operator==(const This& other) const
    {
        return _rel == other._rel and _key == other._key;
    }

    bool operator!=(const This& other) const
    {
        return _rel != other._rel or _key != other._key;
    }

private:
    struct _ExtractItem {
        static boost::python::object Get(const const_iterator& i)
        {
            return boost::python::make_tuple((*i)->GetName(), *i);
        }
    };

    struct _ExtractKey {
        static boost::python::object Get(const const_iterator& i)
        {
            return boost::python::object((*i)->GetName());
        }
    };

    struct _ExtractValue {
        static boost::python::object Get(const const_iterator& i)
        {
            return boost::python::object(*i);
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const boost::python::object owner,
            const const_iterator& cur, const const_iterator& end) :
            _owner(owner),
            _cur(cur),
            _end(end)
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
                TfPyThrowStopIteration("End of RelationalAttributesProxy iteration");
            }
            boost::python::object result = E::Get(_cur);
            ++_cur;
            return result;
        }

    private:
        boost::python::object _owner;
        const_iterator _cur;
        const_iterator _end;
    };

    void _Init()
    {
        TfPyWrapOnce<This>(&This::_Wrap);
    }

    static void _Wrap()
    {
        using namespace boost::python;

        std::string name = _GetName();

        class_<This>(name.c_str(), no_init)
            .def("__repr__", &This::_GetRepr, TfPyRaiseOnError<>())

            .def("__len__", &This::_GetSize, TfPyRaiseOnError<>())
            .def("__getitem__", &This::_GetItemIndex, TfPyRaiseOnError<>())
            .def("__getitem__", &This::_GetItemKey,   TfPyRaiseOnError<>())
            .def("__getitem__", &This::_GetItemSlice, TfPyRaiseOnError<>())
            .def("__setitem__", &This::_SetItemIndex, TfPyRaiseOnError<>())
            .def("__setitem__", &This::_SetItemSlice, TfPyRaiseOnError<>())
            .def("__delitem__", &This::_DelItemIndex, TfPyRaiseOnError<>())
            .def("__delitem__", &This::_DelItemKey,   TfPyRaiseOnError<>())
            .def("__delitem__", &This::_DelItemSlice, TfPyRaiseOnError<>())
            .def("__contains__", &This::_HasKey,   TfPyRaiseOnError<>())
            .def("__contains__", &This::_HasValue, TfPyRaiseOnError<>())
            .def("has_key", &This::_HasKey, TfPyRaiseOnError<>())
            .def("__iter__",   &This::_GetValueIterator,   TfPyRaiseOnError<>())
            .def("itervalues", &This::_GetValueIterator, TfPyRaiseOnError<>())
            .def("iterkeys",   &This::_GetKeyIterator,   TfPyRaiseOnError<>())
            .def("iteritems",  &This::_GetItemIterator,  TfPyRaiseOnError<>())
            .def("values", &This::_GetValues, TfPyRaiseOnError<>())
            .def("keys", &This::_GetKeys,     TfPyRaiseOnError<>())
            .def("items", &This::_GetItems,   TfPyRaiseOnError<>())
            .def("count", &This::_Count,      TfPyRaiseOnError<>())
            .def("index", &This::_FindKey,    TfPyRaiseOnError<>())
            .def("index", &This::_FindValue,  TfPyRaiseOnError<>())
            .def("clear", &This::_Clear,      TfPyRaiseOnError<>())
            .def("insert", &This::_Insert,    TfPyRaiseOnError<>())
            .def("append", &This::_Append,    TfPyRaiseOnError<>())
            .def("remove", &This::_Remove,    TfPyRaiseOnError<>())
            .def("IsValid", &This::IsValid,   TfPyRaiseOnError<>())

            .def(self == self)
            .def(self != self)
            ;

        class_<_Iterator<_ExtractItem> >
            ((name + "_Iterator").c_str(), no_init)
            .def("__iter__", &This::_Iterator<_ExtractItem>::GetCopy)
            .def("next", &This::_Iterator<_ExtractItem>::GetNext)
            ;

        class_<_Iterator<_ExtractKey> >
            ((name + "_KeyIterator").c_str(), no_init)
            .def("__iter__", &This::_Iterator<_ExtractKey>::GetCopy)
            .def("next", &This::_Iterator<_ExtractKey>::GetNext)
            ;

        class_<_Iterator<_ExtractValue> >
            ((name + "_ValueIterator").c_str(), no_init)
            .def("__iter__", &This::_Iterator<_ExtractValue>::GetCopy)
            .def("next", &This::_Iterator<_ExtractValue>::GetNext)
            ;
    }

    static std::string _GetName()
    {
        return TF_PY_REPR_PREFIX + "RelationalAttributesProxy";
    }
    
    std::string _GetRepr() const
    {
        return TfPyRepr(_rel) + ".targetAttributes[" + TfPyRepr(_key) + "]";
    }

    const SdfRelationalAttributeSpecView& _GetView() const
    {
        return _view;
    }

    bool _Validate()
    {
        if (not _GetView().IsValid()) {
            TF_CODING_ERROR("Modifying an expired relational attributes proxy");
            return false;
        }
        return true;
    }

    bool _Validate() const
    {
        if (not _GetView().IsValid()) {
            TF_CODING_ERROR("Accessing an expired relational attributes proxy");
            return false;
        }
        return true;
    }

    int _GetSize() const
    {
        return _Validate() ? _GetView().size() : 0;
    }

    value_type _GetItemIndex(int index) const
    {
        if (_Validate()) {
            const SdfRelationalAttributeSpecView& view = _GetView();

            // Check the index here and throw a python IndexError (expected)
            if ((index < 0) or (static_cast<size_t>(index) >= view.size())) {
                TfPyThrowIndexError("Invalid index");
                return value_type();
            }
            else {
                return view[index];
            }
        }
        else {
            return value_type();
        }
    }

    value_type _GetItemKey(const key_type& key) const
    {
        if (_Validate()) {
            const SdfRelationalAttributeSpecView& view = _GetView();
            const_iterator i = view.find(key);
            if (i == view.end()) {
                TfPyThrowKeyError(TfPyRepr(key));
                return value_type();
            }
            else {
                return *i;
            }
        }
        else {
            return value_type();
        }
    }

    list _GetItemSlice(const boost::python::slice& index) const
    {
        list result;

        if (_Validate()) {
            try {
                const SdfRelationalAttributeSpecView& view = _GetView();

                // XXX:
                // The signature for boost::python::slice::get_indices indicates
                // that a random access iterator is required but from looking
                // at the source, it appears that a bidirectional iterator like
                // the one provided by the view is sufficient.
                slice::range<const_iterator> range =
                    index.get_indicies(view.begin(), view.end());
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

    void _SetItemIndex(int index, const value_type& value)
    {
        if (_Validate()) {
            const SdfRelationalAttributeSpecView& view = _GetView();

            // Check the index here and throw a python IndexError (expected)
            if ((index < 0) or (static_cast<size_t>(index) >= view.size())) {
                TfPyThrowIndexError("Invalid index");
            }
            else {
                value_vector_type attrs = view.values();
                attrs[index] = value;
                _rel->SetAttributesForTargetPath(_key, attrs);
            }
        }
    }

    void _SetItemSlice(const boost::python::slice& index,
                       const value_vector_type& values)
    {
        if (not _Validate()) {
            return;
        }

        const SdfRelationalAttributeSpecView& view = _GetView();
        size_t start, step, count;
        try {
            slice::range<const_iterator> range =
                index.get_indicies(view.begin(), view.end());
            start = range.start - view.begin();
            step  = range.step;
            count = 1 + (range.stop - range.start) / range.step;
        }
        catch (const std::invalid_argument&) {
            // Empty range.
            extract<int> e(index.start());
            start = e.check() ?
                        TfPyNormalizeIndex(e(), view.size(), true) : 0;
            step  = 0;
            count = 0;
        }

        if (TfPyIsNone(index.step())) {
            // Replace contiguous sequence with values.
            value_vector_type attrs = view.values();
            attrs.erase(attrs.begin() + start, attrs.begin() + start + count);
            attrs.insert(attrs.begin() + start, values.begin(), values.end());
            _rel->SetAttributesForTargetPath(_key, attrs);
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
                value_vector_type attrs = view.values();
                std::copy(values.begin(), values.end(),
                          attrs.begin() + start);
                _rel->SetAttributesForTargetPath(_key, attrs);
            }
            else {
                value_vector_type attrs = view.values();
                for (size_t i = 0, j = start; i != count; j += step, ++i) {
                    attrs[j] = values[i];
                }
                _rel->SetAttributesForTargetPath(_key, attrs);
            }
        }
    }

    void _DelItemIndex(int index)
    {
        value_type value = _GetItemIndex(index);
        if (value) {
            _rel->RemoveAttributeForTargetPath(_key, value);
        }
    }

    void _DelItemKey(const key_type& key)
    {
        value_type value = _GetItemKey(key);
        if (value) {
            _rel->RemoveAttributeForTargetPath(_key, value);
        }
    }

    void _DelItemSlice(const boost::python::slice& index)
    {
        if (_Validate()) {
            const SdfRelationalAttributeSpecView& view = _GetView();
            try {
                // Get the range and the number of items in the slice.
                slice::range<const_iterator> range =
                    index.get_indicies(view.begin(), view.end());
                size_t start = range.start - view.begin();
                size_t step  = range.step;
                size_t count = 1 + (range.stop - range.start) / range.step;

                // Erase items.
                if (step == 1) {
                    value_vector_type attrs = view.values();
                    attrs.erase(attrs.begin() + start,
                                attrs.begin() + start + count);
                    _rel->SetAttributesForTargetPath(_key, attrs);
                }
                else {
                    value_vector_type attrs = view.values();
                    for (size_t j = start; count > 0; j += step - 1, --count) {
                        attrs.erase(attrs.begin() + j);
                    }
                    _rel->SetAttributesForTargetPath(_key, attrs);
                }
            }
            catch (const std::invalid_argument&) {
                // Empty slice -- do nothing.
            }
        }
    }

    bool _HasKey(const key_type& key) const
    {
        if (_Validate()) {
            const SdfRelationalAttributeSpecView& view = _GetView();
            return view.find(key) != view.end();
        }
        else {
            return false;
        }
    }

    bool _HasValue(const value_type& value) const
    {
        if (_Validate()) {
            const SdfRelationalAttributeSpecView& view = _GetView();
            const_iterator i = view.find(value);
            return (i != view.end() and *i == value);
        }
        else {
            return false;
        }
    }

    static
    _Iterator<_ExtractKey> _GetKeyIterator(const boost::python::object& obj)
    {
        const This& self = boost::python::extract<const This&>(obj);
        const SdfRelationalAttributeSpecView& view = self._GetView();
        if (self._Validate()) {
            return _Iterator<_ExtractKey>(obj, view.begin(), view.end());
        }
        else {
            return _Iterator<_ExtractKey>(obj, view.end(), view.end());
        }
    }

    static
    _Iterator<_ExtractValue> _GetValueIterator(const boost::python::object& obj)
    {
        const This& self = boost::python::extract<const This&>(obj);
        const SdfRelationalAttributeSpecView& view = self._GetView();
        if (self._Validate()) {
            return _Iterator<_ExtractValue>(obj, view.begin(), view.end());
        }
        else {
            return _Iterator<_ExtractValue>(obj, view.end(), view.end());
        }
    }

    static
    _Iterator<_ExtractItem> _GetItemIterator(const boost::python::object& obj)
    {
        const This& self = boost::python::extract<const This&>(obj);
        const SdfRelationalAttributeSpecView& view = self._GetView();
        if (self._Validate()) {
            return _Iterator<_ExtractItem>(obj, view.begin(), view.end());
        }
        else {
            return _Iterator<_ExtractItem>(obj, view.end(), view.end());
        }
    }

    template <class E>
    list _Get() const
    {
        list result;
        if (_Validate()) {
            const SdfRelationalAttributeSpecView& view = _GetView();
            for (const_iterator i = view.begin(), n = view.end(); i != n; ++i) {
                result.append(E::Get(i));
            }
        }
        return result;
    }

    list _GetValues() const
    {
        return _Get<_ExtractValue>();
    }

    list _GetKeys() const
    {
        return _Get<_ExtractKey>();
    }

    list _GetItems() const
    {
        return _Get<_ExtractItem>();
    }

    int _Count(const value_type& value) const
    {
        return _HasValue(value) ? 1 : 0;
    }

    int _FindKey(const key_type& key) const
    {
        if (_Validate()) {
            const SdfRelationalAttributeSpecView& view = _GetView();
            const_iterator i = view.find(key);
            return i != view.end() ? i - view.begin() : -1;
        }
        else {
            return -1;
        }
    }

    int _FindValue(const value_type& value) const
    {
        if (_Validate()) {
            const SdfRelationalAttributeSpecView& view = _GetView();
            const_iterator i = view.find(value);
            return (i != view.end() and *i == value) ? i - view.begin() : -1;
        }
        else {
            return -1;
        }
    }

    void _Clear()
    {
        if (_Validate()) {
            _rel->SetAttributesForTargetPath(_key, value_vector_type());
        }
    }

    void _Insert(int index, const value_type& value)
    {
        if (_Validate()) {
            // Check the index here to throw a python IndexError rather than
            // allowing _rel to pitch a TF_CODING_ERROR.
            if ((index < -1) or (index > _GetSize())) {
                TfPyThrowIndexError("Invalid index");
            }
            else {
                _rel->InsertAttributeForTargetPath(_key, value, index);
            }
        }
    }

    void _Append(const value_type& value)
    {
        if (_Validate()) {
            _rel->InsertAttributeForTargetPath(_key, value, -1);
        }
    }

    void _Remove(const value_type& value)
    {
        if (_Validate()) {
            _rel->RemoveAttributeForTargetPath(_key, value);
        }
    }

private:
    SdfRelationshipSpecHandle _rel;
    SdfPath _key;
    SdfRelationalAttributeSpecView _view;
};

////////////////////////////////////////////////////////////////////////////////

class Sdf_TargetAttributeMapProxy
{
public:
    typedef Sdf_TargetAttributeMapProxy This;
    typedef SdfPath key_type;
    typedef std::vector<key_type> key_vector_type;
    typedef Sdf_RelationalAttributesProxy value_type;
    typedef std::vector<value_type> value_vector_type;
    
    explicit Sdf_TargetAttributeMapProxy(const SdfRelationshipSpecHandle& rel) :
        _rel(rel)
    {
        _Init();
    }
    
private:
    void _Init()
    {
        TfPyWrapOnce<This>(&This::_Wrap);
    }

    static void _Wrap()
    {
        using namespace boost::python;

        std::string name = _GetName();

        class_<This>(name.c_str(), no_init)
            .def("__repr__", &This::_GetRepr, TfPyRaiseOnError<>())
            .def("__len__", &This::_GetSize, TfPyRaiseOnError<>())
            .def("__getitem__", &This::_GetItemByKey, TfPyRaiseOnError<>())
            .def("__contains__", &This::_ContainsItemWithKey,
                 TfPyRaiseOnError<>())
            .def("get", &This::_GetItemOrNone, TfPyRaiseOnError<>())
            .def("has_key", &This::_ContainsItemWithKey, TfPyRaiseOnError<>())
            .def("items", &This::_GetItems, TfPyRaiseOnError<>())
            .def("keys", &This::_GetKeys,
                 TfPyRaiseOnError<return_value_policy<TfPySequenceToList> >())
            .def("values", &This::_GetValues,
                 TfPyRaiseOnError<return_value_policy<TfPySequenceToList> >())
            ;
    }
    
    static std::string _GetName()
    {
        return TF_PY_REPR_PREFIX + "TargetAttributeMapProxy";
    }
    
    std::string _GetRepr() const
    {
        return TfPyRepr(_rel) + ".targetAttributes";
    }

    value_type _GetValue(const key_type& key) const
    {
        return value_type(_rel, key);
    }

    size_t _GetSize() const
    {
        return _CheckRelationshipValidity() ? _GetKeys().size() : 0;
    }
    
    value_type _GetItemByKey(const key_type& key) const
    {
        return _CheckRelationshipValidity() ? _GetValue(key) : value_type();
    }
    
    bool _ContainsItemWithKey(const key_type& key) const
    {
        return _CheckRelationshipValidity() ?
                    not _rel->GetAttributesForTargetPath(key).empty() : false;
    }
    
    boost::python::object _GetItemOrNone(const key_type& key) const
    {
        value_type value = _GetItemByKey(key);
        
        if (not value.IsValid()) {
            return boost::python::object();
        }
        else {
            return boost::python::object(value);
        }
    }
    
    boost::python::list _GetItems() const
    {
        boost::python::list items;
        if (_CheckRelationshipValidity()) {
            key_vector_type keys = _GetKeys();
            TF_FOR_ALL(keyIt, keys) {
                items.append(
                    boost::python::make_tuple(*keyIt, _GetValue(*keyIt)));
            }
        }
        return items;
    }
    
    key_vector_type _GetKeys() const
    {
        if (_CheckRelationshipValidity()) {
            return _rel->GetAttributeTargetPaths();
        }
        else {
            return key_vector_type();
        }
    }
    
    value_vector_type _GetValues() const
    {
        value_vector_type values;
        if (_CheckRelationshipValidity()) {
            key_vector_type keys = _GetKeys();
            TF_FOR_ALL(keyIt, keys) {
                values.push_back(_GetValue(*keyIt));
            }
        }
        return values;
    }
    
    bool _CheckRelationshipValidity() const
    {
        if (not _rel) {
            TF_CODING_ERROR("Accessing an expired relationship");
            return false;
        }
        return true;
    }

private:
    SdfRelationshipSpecHandle _rel;
};

static
Sdf_TargetAttributeMapProxy
_WrapGetRelationalAttributes(const SdfRelationshipSpecHandle& rel)
{
    return Sdf_TargetAttributeMapProxy(rel);
}

////////////////////////////////////////////////////////////////////////////////

class Sdf_TargetAttributeOrderMapProxy {
public:
    typedef SdfPath key_type;
    typedef SdfNameOrderProxy mapped_type;
    typedef size_t size_type;
    typedef Sdf_TargetAttributeOrderMapProxy This;

    Sdf_TargetAttributeOrderMapProxy(const SdfRelationshipSpecHandle& rel)
        : _rel(rel)
    {
        _Init();
    }

    bool operator==(const This& other) const
    {
        return _rel == other._rel;
    }

    bool operator!=(const This& other) const
    {
        return _rel != other._rel;
    }

private:
    struct _ExtractItem {
        static boost::python::object Get(const SdfRelationshipSpecHandle& rel,
                                         const SdfPath& targetPath)
        {
            return boost::python::make_tuple(
                targetPath, 
                rel->GetOrCreateAttributeOrderForTargetPath(targetPath));
        }
    };

    struct _ExtractKey {
        static boost::python::object Get(const SdfRelationshipSpecHandle& rel,
                                         const SdfPath& targetPath)
        {
            return boost::python::object(targetPath);
        }
    };

    struct _ExtractValue {
        static boost::python::object Get(const SdfRelationshipSpecHandle& rel,
                                         const SdfPath& targetPath)
        {
            return boost::python::object(
                rel->GetOrCreateAttributeOrderForTargetPath(targetPath));
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const SdfRelationshipSpecHandle& rel)
            : _rel(rel)
        {
            if (_rel) {
                const SdfPathVector paths = _rel->GetAttributeOrderTargetPaths();
                _orderTargetPaths.insert(
                    _orderTargetPaths.end(), paths.begin(), paths.end());
            }
        }

        _Iterator<E> GetCopy() const
        {
            return *this;
        }

        boost::python::object GetNext()
        {
            if (_orderTargetPaths.empty()) {
                TfPyThrowStopIteration("End of attribute order iteration");
            }
            boost::python::object result = 
                E::Get(_rel, _orderTargetPaths.front());
            _orderTargetPaths.pop_front();
            return result;
        }

    private:
        SdfRelationshipSpecHandle _rel;
        std::deque<SdfPath> _orderTargetPaths;
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
            .def("__setitem__", &This::_SetItemByKey, TfPyRaiseOnError<>())
            .def("__delitem__", &This::_DelItemByKey, TfPyRaiseOnError<>())
            .def("__contains__", &This::_HasKey, TfPyRaiseOnError<>())
            .def("__iter__",   &This::_GetItemIterator, TfPyRaiseOnError<>())
            .def("itervalues", &This::_GetValueIterator, TfPyRaiseOnError<>())
            .def("iterkeys",   &This::_GetKeyIterator, TfPyRaiseOnError<>())
            .def("iteritems",  &This::_GetItemIterator, TfPyRaiseOnError<>())
            .def("clear", &This::_Clear, TfPyRaiseOnError<>())
            .def("get", &This::_PyGet, TfPyRaiseOnError<>())
            .def("has_key", &This::_HasKey, TfPyRaiseOnError<>())
            .def("items", &This::_GetItems, TfPyRaiseOnError<>())
            .def("keys", &This::_GetKeys, TfPyRaiseOnError<>())
            .def("values", &This::_GetValues, TfPyRaiseOnError<>())
            .def("__eq__", &This::operator==, TfPyRaiseOnError<>())
            .def("__ne__", &This::operator!=, TfPyRaiseOnError<>())
            ;

        class_<_Iterator<_ExtractItem> >
            ((name + "_Iterator").c_str(), no_init)
            .def("__iter__", &This::_Iterator<_ExtractItem>::GetCopy)
            .def("next", &This::_Iterator<_ExtractItem>::GetNext)
            ;

        class_<_Iterator<_ExtractKey> >
            ((name + "_KeyIterator").c_str(), no_init)
            .def("__iter__", &This::_Iterator<_ExtractKey>::GetCopy)
            .def("next", &This::_Iterator<_ExtractKey>::GetNext)
            ;

        class_<_Iterator<_ExtractValue> >
            ((name + "_ValueIterator").c_str(), no_init)
            .def("__iter__", &This::_Iterator<_ExtractValue>::GetCopy)
            .def("next", &This::_Iterator<_ExtractValue>::GetNext)
            ;
    }

    static std::string _GetName()
    {
        return TF_PY_REPR_PREFIX + "TargetAttributesOrderProxy";
    }

    std::string _GetRepr() const
    {
        std::string result("{");

        if (_Validate()) {
            const SdfPathVector paths = _rel->GetAttributeOrderTargetPaths();
            if (not paths.empty()) {
                for (size_t i = 0; i < paths.size(); ++i) {
                    if (i != 0) {
                        result += ", ";
                    }
                    
                    SdfNameOrderProxy orderProxy = 
                        _rel->GetOrCreateAttributeOrderForTargetPath(paths[i]);
                    result += TfPyRepr(paths[i]) + ": " + TfPyRepr(orderProxy);
                }
            }
        }

        result += "}";
        return result;
    }

    size_type _GetSize() const
    {
        return _Validate() ? _rel->GetAttributeOrderTargetPaths().size() : 0;
    }

    mapped_type _GetItemByKey(const key_type& key) const
    {
        if (not _Validate()) {
            return mapped_type(SdfListOpTypeOrdered);
        }

        if (not _rel->HasAttributeOrderForTargetPath(key)) {
            TfPyThrowKeyError(TfPyRepr(key));
        }

        return _rel->GetOrCreateAttributeOrderForTargetPath(key);
    }

    void _SetItemByKey(const key_type& key, 
                       const std::vector<std::string>& value)
    {
        if (not _Validate()) {
            return;
        }

        SdfChangeBlock block;
        _rel->GetOrCreateAttributeOrderForTargetPath(key) = value;
    }

    void _DelItemByKey(const key_type& key)
    {
        if (not _Validate()) {
            return;
        }

        SdfChangeBlock block;
        _rel->GetOrCreateAttributeOrderForTargetPath(key).clear();
    }

    void _Clear()
    {
        if (not _Validate()) {
            return;
        }

        SdfChangeBlock block;
        const SdfPathVector paths = _rel->GetAttributeOrderTargetPaths();
        TF_FOR_ALL(path, paths) {
            _rel->GetOrCreateAttributeOrderForTargetPath(*path).clear();
        }
    }

    boost::python::object _PyGet(const key_type& key) const
    {
        if (not _Validate() or
            not _rel->HasAttributeOrderForTargetPath(key)) {
            return boost::python::object();
        }

        return boost::python::object(
            _rel->GetOrCreateAttributeOrderForTargetPath(key));
    }

    bool _HasKey(const key_type& key) const
    {
        return _Validate() ? _rel->HasAttributeOrderForTargetPath(key) : false;
    }

    _Iterator<_ExtractItem> _GetItemIterator() const
    {
        // Just issue error if invalid; _Iterator will handle the rest.
        _Validate(); 
        return _Iterator<_ExtractItem>(_rel);
    }

    _Iterator<_ExtractKey> _GetKeyIterator() const
    {
        // Just issue error if invalid; _Iterator will handle the rest.
        _Validate();
        return _Iterator<_ExtractKey>(_rel);
    }

    _Iterator<_ExtractValue> _GetValueIterator() const
    {
        // Just issue error if invalid; _Iterator will handle the rest.
        _Validate();
        return _Iterator<_ExtractValue>(_rel);
    }

    template <class E>
    boost::python::list _Get() const
    {
        boost::python::list result;

        if (_Validate()) {
            const SdfPathVector paths = _rel->GetAttributeOrderTargetPaths();
            TF_FOR_ALL(path, paths) {
                result.append(E::Get(_rel, *path));
            }
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

    bool _Validate() const
    {
        if (not _rel) {
            TF_CODING_ERROR("Accessing an expired relationship");
            return false;
        }
        return true;
    }

private:
    SdfRelationshipSpecHandle _rel;
};

static
Sdf_TargetAttributeOrderMapProxy
_WrapGetTargetAttributeOrders(const SdfRelationshipSpec& spec)
{
    SdfRelationshipSpecHandle rel(spec);
    return Sdf_TargetAttributeOrderMapProxy(rel);
}

static
void
_WrapSetTargetAttributeOrders(
    SdfRelationshipSpec& rel,
    const dict& d)
{
    std::map< SdfPath, std::vector<TfToken> > orders;

    list keys = d.keys();
    int numKeys = len(d);

    for (int i = 0; i < numKeys; i++) {
        SdfPath key = extract<SdfPath>(keys[i]);
        list l = extract<list>(d[keys[i]]);
        
        int numAttrNames = len(l);
        std::vector<TfToken> attrNameVec;
        for (int j = 0; j < numAttrNames; j++) {
            TfToken val = extract<TfToken>(l[j]);
            attrNameVec.push_back(val);
        }
        
        orders[key] = attrNameVec;
    }

    rel.SetTargetAttributeOrders(orders);
}

////////////////////////////////////////////////////////////////////////////////

template <>
class Sdf_PyMarkerPolicy<SdfRelationshipSpec> 
{
public:
    static SdfPathVector GetMarkerPaths(const SdfRelationshipSpecHandle& spec)
    {
        return spec->GetTargetMarkerPaths();
    }

    static std::string GetMarker(const SdfRelationshipSpecHandle& spec,
                                 const SdfPath& path)
    {
        return spec->GetTargetMarker(path);
    }

    static void SetMarker(const SdfRelationshipSpecHandle& spec,
                          const SdfPath& path, const std::string& marker)
    {
        spec->SetTargetMarker(path, marker);
    }

    static void SetMarkers(const SdfRelationshipSpecHandle& spec,
                           const std::map<SdfPath, std::string>& markers)
    {
        SdfRelationshipSpec::TargetMarkerMap m(markers.begin(), markers.end());
        spec->SetTargetMarkers(m);
    }
};

static
SdfPyMarkerProxy<SdfRelationshipSpec>
_WrapGetMarkers(const SdfRelationshipSpec& spec)
{
    SdfRelationshipSpecHandle rel(spec);
    return SdfPyMarkerProxy<SdfRelationshipSpec>(rel);
}

static
void
_WrapSetMarkers(
    SdfRelationshipSpec& rel,
    const dict& d)
{
    SdfRelationshipSpec::TargetMarkerMap markers;

    list keys = d.keys();
    int numKeys = len(d);

    for (int i = 0; i < numKeys; i++) {
        SdfPath key = extract<SdfPath>(keys[i]);
        std::string val = extract<std::string>(d[keys[i]]);

        markers[key] = val;
    }

    rel.SetTargetMarkers(markers);
}

////////////////////////////////////////////////////////////////////////////////

static
SdfPath
_WrapGetTargetPathForAttribute(
    SdfRelationshipSpec& rel,
    const SdfAttributeSpecHandle& attr)
{
    // This will use a conversion constructor to convert the Python-held
    // SdfHandle<SdfAttributeSpec> into the required
    // SdfHandle<const SdfAttributeSpec>
    return rel.GetTargetPathForAttribute(attr);
}

static
bool
_WrapInsertAttributeForTargetPath(
    SdfRelationshipSpec& rel,
    const SdfPath& path,
    const SdfAttributeSpecHandle& attr)
{
    return rel.InsertAttributeForTargetPath(path, attr);
}

static
bool
_WrapInsertAttributeForTargetPathWithIndex(
    SdfRelationshipSpec& rel,
    const SdfPath& path,
    const SdfAttributeSpecHandle& attr,
    int index)
{
    return rel.InsertAttributeForTargetPath(path, attr, index);
}

////////////////////////////////////////////////////////////////////////////////

void wrapRelationshipSpec()
{    
    typedef SdfRelationshipSpec This;

    TfPyContainerConversions::from_python_sequence<
        std::vector< SdfAttributeSpecHandle >,
        TfPyContainerConversions::variable_capacity_policy >();

    class_<This, SdfHandle<This>, 
           bases<SdfPropertySpec>, boost::noncopyable>
        ("RelationshipSpec", no_init)
        
        .def(SdfPySpec())

        .def("__unused__", 
            SdfMakePySpecConstructor(&This::New,
                "__init__(ownerPrimSpec, name, custom = True, variability = "
                "Sd.VariabilityUniform)\n"
                "ownerPrimSpec: PrimSpec\n"
                "name : string\n"
                "custom : bool\n"
                "varibility : Sd.Variability\n"),
                (arg("ownerPrimSpec"),
                 arg("name"),
                 arg("custom") = true,
                 arg("variability") = SdfVariabilityUniform))

        .add_property("targetPathList",
            &This::GetTargetPathList,
            "A PathListEditor for the relationship's target paths.\n\n"
            "The list of the target paths for this relationship may be\n"
            "modified with this PathListEditor.\n\n"
            "A PathListEditor may express a list either as an explicit \n"
            "value or as a set of list editing operations.  See PathListEditor \n"
            "for more information.")

        .add_property("targetAttributes",
            &::_WrapGetRelationalAttributes,
            "A dictionary of the attributes for each target path, keyed by path.\n\n"
            "Each dictionary value is a dictionary of attributes,"
            "keyed by attribute name.  The targetAttributes property itself "
            "is read-only, but the attributes for a particular target may be "
            "modified just as you might modify a prim's attributes.")

        .add_property("targetAttributeOrders",
            &::_WrapGetTargetAttributeOrders,
            &::_WrapSetTargetAttributeOrders,
            "A dictionary of relational attribute order name lists for each "
            "target path, keyed by path.\n\n")

        .add_property("targetMarkers",
            &::_WrapGetMarkers,
            &::_WrapSetMarkers,
            "The markers for this relationship in a map proxy\n"
            "keyed by target path.\n\n"
            "The returned proxy can be used to set or remove the\n"
            "marker for a given path or to access the markers.")

        .add_property("noLoadHint",
            &This::GetNoLoadHint,
            &This::SetNoLoadHint,
            "whether the target must be loaded to load the prim this\n"
            "relationship is attached to.")

        .def("GetTargetPathForAttribute", &::_WrapGetTargetPathForAttribute)
        .def("ReplaceTargetPath", &This::ReplaceTargetPath)
        .def("RemoveTargetPath", &This::RemoveTargetPath,
             (arg("preserveTargetOrder") = false))
        .def("InsertAttributeForTargetPath",
             &::_WrapInsertAttributeForTargetPath)
        .def("InsertAttributeForTargetPath",
             &::_WrapInsertAttributeForTargetPathWithIndex)

        .def("HasAttributeOrderForTargetPath",
             &This::HasAttributeOrderForTargetPath)
        .def("GetAttributeOrderForTargetPath",
             &This::GetAttributeOrderForTargetPath)
        .def("GetOrCreateAttributeOrderForTargetPath",
             &This::GetOrCreateAttributeOrderForTargetPath)
        .def("GetAttributeOrderTargetPaths",
             &This::GetAttributeOrderTargetPaths)

        .def("GetTargetMarker", &This::GetTargetMarker)
        .def("SetTargetMarker", &This::SetTargetMarker)
        .def("ClearTargetMarker", &This::ClearTargetMarker)
        .def("GetTargetMarkerPaths", &This::GetTargetMarkerPaths)

        // property keys
        .setattr("TargetsKey", SdfFieldKeys->TargetPaths)
        ;

}
