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

#include "pxr/pxr.h"
#include "pxr/usd/sdf/mapEditor.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

//
// Sdf_MapEditor<T>
//

template <class T>
Sdf_MapEditor<T>::Sdf_MapEditor()
{
}

template <class T>
Sdf_MapEditor<T>::~Sdf_MapEditor()
{
}

//
// Sdf_LsdMapEditor<T>
//

template <class T>
class Sdf_LsdMapEditor : 
    public Sdf_MapEditor<T>
{
public:
    typedef typename Sdf_MapEditor<T>::key_type key_type;
    typedef typename Sdf_MapEditor<T>::mapped_type mapped_type;
    typedef typename Sdf_MapEditor<T>::value_type value_type;
    typedef typename Sdf_MapEditor<T>::iterator iterator;

    Sdf_LsdMapEditor(const SdfSpecHandle& owner, const TfToken& field) :
        _owner(owner),
        _field(field)
    {
        const VtValue& dataVal = _owner->GetField(_field);
        if (!dataVal.IsEmpty()) {
            if (dataVal.IsHolding<T>()) {
                _data = dataVal.Get<T>();
            }
            else {
                TF_CODING_ERROR("%s does not hold value of expected type.",
                                GetLocation().c_str());
            }
        }
    }

    virtual std::string GetLocation() const
    {
        return TfStringPrintf("field '%s' in <%s>", 
                              _field.GetText(), _owner->GetPath().GetText());
    }

    virtual SdfSpecHandle GetOwner() const
    {
        return _owner;
    }

    virtual bool IsExpired() const
    {
        return !_owner;
    }

    virtual const T* GetData() const
    {
        return &_data;
    }

    virtual T* GetData()
    {
        return &_data;
    }

    virtual void Copy(const T& other)
    {
        _data = other;
        _UpdateDataInSpec();
    }

    virtual void Set(const key_type& key, const mapped_type& other)
    {
        _data[key] = other;
        _UpdateDataInSpec();
    }

    virtual std::pair<iterator, bool> Insert(const value_type& value)
    {
        const std::pair<iterator, bool> insertStatus = _data.insert(value);
        if (insertStatus.second) {
            _UpdateDataInSpec();
        }

        return insertStatus;
    }

    virtual bool Erase(const key_type& key)
    {
        const bool didErase = (_data.erase(key) != 0);
        if (didErase) {
            _UpdateDataInSpec();
        }

        return didErase;
    }

    virtual SdfAllowed IsValidKey(const key_type& key) const
    {
        if (const SdfSchema::FieldDefinition* def = 
                _owner->GetSchema().GetFieldDefinition(_field)) {
            return def->IsValidMapKey(key);
        }
        return true;
    }

    virtual SdfAllowed IsValidValue(const mapped_type& value) const
    {
        if (const SdfSchema::FieldDefinition* def = 
                _owner->GetSchema().GetFieldDefinition(_field)) {
            return def->IsValidMapValue(value);
        }
        return true;
    }

private:
    void _UpdateDataInSpec()
    {
        TfAutoMallocTag2 tag("Sdf", "Sdf_LsdMapEditor::_UpdateDataInSpec");

        if (TF_VERIFY(_owner)) {
            if (_data.empty()) {
                _owner->ClearField(_field);
            }
            else {
                _owner->SetField(_field, _data);
            }
        }
    }

private:
    SdfSpecHandle _owner;
    TfToken _field;
    T _data;
};

//
// Factory functions
//

template <class T>
boost::shared_ptr<Sdf_MapEditor<T> > 
Sdf_CreateMapEditor(const SdfSpecHandle& owner, const TfToken& field)
{
    return boost::shared_ptr<Sdf_MapEditor<T> >(
        new Sdf_LsdMapEditor<T>(owner, field));
}

//
// Template instantiations
//

#define SDF_INSTANTIATE_MAP_EDITOR(MapType)                          \
    template class Sdf_MapEditor<MapType>;                           \
    template class Sdf_LsdMapEditor<MapType>;                        \
    template boost::shared_ptr<Sdf_MapEditor<MapType> >              \
        Sdf_CreateMapEditor(const SdfSpecHandle&, const TfToken&);   \

#include "pxr/base/vt/dictionary.h"
#include "pxr/usd/sdf/types.h"
SDF_INSTANTIATE_MAP_EDITOR(VtDictionary); 
SDF_INSTANTIATE_MAP_EDITOR(SdfVariantSelectionMap); 
SDF_INSTANTIATE_MAP_EDITOR(SdfRelocatesMap); 

PXR_NAMESPACE_CLOSE_SCOPE
