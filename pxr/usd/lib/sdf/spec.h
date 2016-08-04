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
/// \file sdf/spec.h

#ifndef SDF_SPEC_H
#define SDF_SPEC_H

#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/identity.h"
#include "pxr/usd/sdf/declareSpec.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include <boost/type_traits/is_base_of.hpp>
#include <boost/utility/enable_if.hpp>
#include <iosfwd>

/// \class SdfSpec
/// \brief Base class for all Sdf spec classes.
///
class SdfSpec {
    SDF_DECLARE_BASE_SPEC(SdfSchema, SdfSpec);

public:
    SDF_API
    SdfSpec &operator=(const SdfSpec &other);

    SDF_API
    ~SdfSpec();

    /// \name SdSpec generic API
    ///
    /// @{

    /// \brief Returns the SdfSchemaBase for the layer that owns this spec.
    SDF_API
    const SdfSchemaBase& GetSchema() const;

    /// \brief Returns the SdfSpecType specifying the spec type this object
    /// represents.
    SDF_API
    SdfSpecType GetSpecType() const;

    /// \brief Returns true if this object is invalid or expired.
    SDF_API
    bool IsDormant() const;

    /// \brief Returns the layer that this object belongs to.
    SDF_API
    SdfLayerHandle GetLayer() const;

    /// \brief Returns the scene path of this object.
    SDF_API
    SdfPath GetPath() const;

    /// \brief Returns whether this object's layer can be edited.
    SDF_API
    bool PermissionToEdit() const;

    /// Returns the full list of info keys currently set on this object.
    /// \note This does not include fields that represent names of children.
    SDF_API
    std::vector<TfToken> ListInfoKeys() const;

    /// \brief Returns the list of metadata info keys for this object.
    ///
    /// This is not the complete list of keys, it is only those that
    /// should be considered to be metadata by inspectors or other 
    /// presentation UI.
    ///
    /// This is interim API which is likely to change.  Only editors with
    /// an immediate specific need (like the Inspector) should use this API.
    SDF_API
    std::vector<TfToken> GetMetaDataInfoKeys() const;

    /// \brief Returns this metadata key's displayGroup.
    ///
    SDF_API
    TfToken GetMetaDataDisplayGroup(TfToken const &key) const;

    /// \brief Gets the value for the given metadata key.
    ///
    /// This is interim API which is likely to change.  Only editors with
    /// an immediate specific need (like the Inspector) should use this API.
    SDF_API
    VtValue GetInfo(const TfToken &key) const;

    /// \brief Sets the value for the given metadata key.
    ///
    /// It is an error to pass a value that is not the correct type for
    /// that given key.
    ///
    /// This is interim API which is likely to change.  Only editors with
    /// an immediate specific need (like the Inspector) should use this API.
    SDF_API
    void SetInfo(const TfToken &key, const VtValue &value);

    /// \brief Sets the value for \p entryKey to \p value within the dictionary 
    ///        with the given metadata key \p dictionaryKey
    SDF_API
    void SetInfoDictionaryValue(const TfToken &dictionaryKey, 
                                const TfToken &entryKey, const VtValue &value);

    /// \brief Returns whether there is a setting for the scene spec info 
    /// with the given key.
    ///
    /// When asked for a value for one of its scene spec info, a valid value
    /// will always be returned. But if this API returns \b false for a scene
    /// spec info, the value of that info will be the defined default value.
    ///
    /// When dealing with a composedLayer, it is not necessary to worry about
    /// whether a scene spec info "has a value" because the composed layer will
    /// always have a valid value, even if it is the default.
    ///
    /// A spec may or may not have an expressed value for some of its
    /// scene spec info.
    ///
    /// This is interim API which is likely to change.  Only editors with
    /// an immediate specific need (like the Inspector) should use this API.
    SDF_API
    bool HasInfo(const TfToken &key) const;

    /// \brief Clears the value for scene spec info with the given \a key.
    ///
    /// After calling this, HasInfo() will return \b false.
    /// To make HasInfo() return \b true just set a value for that
    /// scene spec info.
    ///
    /// This is interim API which is likely to change.  Only editors with
    /// an immediate specific need (like the Inspector) should use this API.
    SDF_API
    void ClearInfo(const TfToken &key);

    /// Returns the data type for the info with the given \a key.
    SDF_API
    TfType GetTypeForInfo(const TfToken &key) const;

    /// Returns the fallback for the info with the given \a key.
    SDF_API
    const VtValue& GetFallbackForInfo(const TfToken &key) const;

    /// Writes this spec to the given stream.
    SDF_API
    bool WriteToStream(std::ostream&, size_t indent = 0) const;

    /// \brief Returns whether this object has no significant data.
    ///
    /// "Significant" here means that the object contributes opinions to
    /// a scene. If this spec has any child scenegraph objects (e.g.,
    /// prim or property spec), it will be considered significant even if
    /// those child objects are not.
    /// However, if \p ignoreChildren is \c true, these child objects
    /// will be ignored. 
    SDF_API
    bool IsInert(bool ignoreChildren = false) const;

    /// @}


    /// \name Field-based Generic API
    ///
    /// @{

    /// Returns all fields with values.
    SDF_API
    std::vector<TfToken> ListFields() const;

    /// Returns \c true if the spec has a non-empty value with field
    /// name \p name.
    SDF_API
    bool HasField(const TfToken &name) const;

    /// Returns \c true if the object has a non-empty value with name
    /// \p name and type \p T.  If value ptr is provided, returns the
    /// value found.
    template <class T>
    bool HasField(const TfToken &name, T* value) const
    {
        if (not value) {
            return HasField(name);
        }
        
        SdfAbstractDataTypedValue<T> outValue(value);
        return _HasField(name, &outValue);
    }

    /// Returns a field value by name.
    SDF_API
	VtValue GetField(const TfToken &name) const;

    /// Returns a field value by name.  If the object is invalid, or the
    /// value doesn't exist, isn't set, or isn't of the given type then
    /// returns defaultValue.
    template <typename T>
    T GetFieldAs(const TfToken & name, const T& defaultValue = T()) const
    {
        VtValue v = GetField(name);
        if (v.IsEmpty() or not v.IsHolding<T>())
            return defaultValue;
        return v.UncheckedGet<T>();
    }

    /// Sets a field value as a boxed VtValue.
    SDF_API
	bool SetField(const TfToken & name, const VtValue& value);

    /// Sets a field value of type T.
    template <typename T>
    bool SetField(const TfToken & name, const T& value)
    {
        return SetField(name, VtValue(value));
    }

    /// Clears a field.
    SDF_API
    bool ClearField(const TfToken & name);

    /// @}

    /// \name Comparison operators
    /// @{

    SDF_API bool operator==(const SdfSpec& rhs) const;
    SDF_API bool operator<(const SdfSpec& rhs) const;

    /// @}

private:
    bool _HasField(const TfToken& name, SdfAbstractDataValue* value) const;

protected:
    bool _MoveSpec(const SdfPath &oldPath, const SdfPath &newPath) const;
    bool _DeleteSpec(const SdfPath &path);

private:
    Sdf_IdentityRefPtr _id;

};

#endif
