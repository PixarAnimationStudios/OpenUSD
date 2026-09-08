//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_MAP_EDITOR_H
#define PXR_USD_SDF_MAP_EDITOR_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/allowed.h"
#include "pxr/usd/sdf/spec.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/token.h"

#include <memory>
#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class VtValueRef;

/// \class Sdf_MapEditorBase
///
/// Non-template base for Sdf_MapEditor<T> holding the type-independent
/// owner/field state and operations.
///
class Sdf_MapEditorBase {
public:
    /// Returns a string describing the location of the map being edited.
    /// Used for debugging and error messages.
    SDF_API std::string GetLocation() const;

    /// Returns owner of the map being edited.
    SDF_API SdfSpecHandle GetOwner() const;

    /// Returns true if the map being edited is expired, false otherwise.
    SDF_API bool IsExpired() const;

protected:
    SDF_API Sdf_MapEditorBase(const SdfSpecHandle& owner, const TfToken& field);

    SDF_API SdfAllowed _IsValidKey(const VtValueRef& key) const;
    SDF_API SdfAllowed _IsValidValue(const VtValueRef& value) const;
    SDF_API void _WriteToSpec(const VtValueRef& value);

    SdfSpecHandle _owner;
    TfToken _field;
};

/// \class Sdf_MapEditor
///
/// Private implementation used by SdfMapEditProxy.
///
template <class MapType>
class Sdf_MapEditor : public Sdf_MapEditorBase {
public:
    typedef typename MapType::key_type    key_type;
    typedef typename MapType::mapped_type mapped_type;
    typedef typename MapType::value_type  value_type;
    typedef typename MapType::iterator    iterator;

    Sdf_MapEditor(const SdfSpecHandle& owner, const TfToken& field)
        : Sdf_MapEditorBase(owner, field)
    {
        const VtValue& dataVal = _owner->GetField(_field);
        if (!dataVal.IsEmpty()) {
            if (dataVal.IsHolding<MapType>()) {
                _data = dataVal.Get<MapType>();
            }
            else {
                TF_CODING_ERROR("%s does not hold value of expected type.",
                                GetLocation().c_str());
            }
        }
    }

    const MapType* GetData() const { return &_data; }
    MapType* GetData()             { return &_data; }

    /// \name Editing Operations
    /// @{

    void Copy(const MapType& other)
    {
        _data = other;
        _UpdateDataInSpec();
    }

    void Set(const key_type& key, const mapped_type& other)
    {
        _data[key] = other;
        _UpdateDataInSpec();
    }

    std::pair<iterator, bool> Insert(const value_type& value)
    {
        const std::pair<iterator, bool> insertStatus = _data.insert(value);
        if (insertStatus.second) {
            _UpdateDataInSpec();
        }
        return insertStatus;
    }

    bool Erase(const key_type& key)
    {
        const bool didErase = (_data.erase(key) != 0);
        if (didErase) {
            _UpdateDataInSpec();
        }
        return didErase;
    }

    SdfAllowed IsValidKey(const key_type& key) const
    {
        return _IsValidKey(VtValueRef(key));
    }

    SdfAllowed IsValidValue(const mapped_type& value) const
    {
        return _IsValidValue(VtValueRef(value));
    }

    /// @}

private:
    void _UpdateDataInSpec()
    {
        _WriteToSpec(_data.empty() ? VtValueRef() : VtValueRef(_data));
    }

    MapType _data;
};

template <class T>
std::unique_ptr<Sdf_MapEditor<T>>
Sdf_CreateMapEditor(const SdfSpecHandle& owner, const TfToken& field)
{
    return std::make_unique<Sdf_MapEditor<T>>(owner, field);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_MAP_EDITOR_H
