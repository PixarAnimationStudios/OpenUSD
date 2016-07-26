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
#ifndef USD_OBJECT_H
#define USD_OBJECT_H

#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/primData.h"

#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/path.h"

#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_base_of.hpp>

TF_DECLARE_WEAK_PTRS(UsdStage);

/// Enum values to represent the various Usd object types.
enum UsdObjType
{
    // Value order matters in this enum.
    UsdTypeObject,
    UsdTypePrim,
    UsdTypeProperty,
    UsdTypeAttribute,
    UsdTypeRelationship,

    Usd_NumObjTypes
};


namespace _Detail {

// A metafunction that takes a UsdObject class like UsdObject, UsdPrim,
// UsdProperty, etc, and gives its corresponding UsdObjType, e.g. UsdTypeObject,
// UsdTypePrim, UsdTypeProperty, etc.  Usage: GetObjType<UsdPrim>::Value.
template <UsdObjType Type>
struct Const { static const UsdObjType Value = Type; };
template <class T> struct GetObjType {
    BOOST_MPL_ASSERT_MSG(false,
                         Type_must_be_UsdObject_subclass, (T));
};
template <> struct GetObjType<UsdObject> : Const<UsdTypeObject> {};
template <> struct GetObjType<UsdPrim> : Const<UsdTypePrim> {};
template <> struct GetObjType<UsdProperty> : Const<UsdTypeProperty> {};
template <> struct GetObjType<UsdAttribute> : Const<UsdTypeAttribute> {};
template <> struct GetObjType<UsdRelationship> : Const<UsdTypeRelationship> {};

} // _Detail

/// Return true if \a subType is the same as or a subtype of \a baseType, false
/// otherwise.
inline bool
UsdIsSubtype(UsdObjType baseType, UsdObjType subType) {
    return (baseType == UsdTypeObject) or (baseType == subType) or
        (baseType == UsdTypeProperty and subType > UsdTypeProperty);
}

/// Return true if \a from is convertible to \a to, false otherwise.  Equivalent
/// to UsdIsSubtype(to, from).
inline bool
UsdIsConvertible(UsdObjType from, UsdObjType to) {
    return UsdIsSubtype(to, from);
}

/// Return true if \a type is a concrete object type, namely one of Prim,
/// Attribute, or Relationship.
inline bool
UsdIsConcrete(UsdObjType type) {
    return type == UsdTypePrim or
        type == UsdTypeAttribute or
        type == UsdTypeRelationship;
}

/// \class UsdObject
/// \brief Base class for Usd scenegraph objects, providing common API.
///
/// The commonality between the three types of scenegraph objects in Usd
/// (\ref UsdPrim, \ref UsdAttribute, \ref UsdRelationship) is that they can
/// all have metadata.  Other objects in the API (\ref UsdReferences, 
/// \ref UsdVariantSets, etc.) simply \em are kinds of metadata.
///
/// UsdObject's API primarily provides schema for interacting with the metadata
/// common to all the scenegraph objects, as well as generic access to metadata.
///
/// section Usd_UsdObject_Lifetime Lifetime Management and Object Validity
///
/// Every derived class of UsdObject supports explicit detection of object
/// validity through an \em unspecified-bool-type operator 
/// (i.e. safe bool conversion), so client code should always be able use
/// objects safely, even across edits to the owning UsdStage.  UsdObject
/// classes also perform some level of validity checking upon every use, in
/// order to facilitate debugging of unsafe code, although we reserve the right
/// to activate that behavior only in debug builds, if it becomes compelling
/// to do so for performance reasons.  This per-use checking will cause a
/// fatal error upon failing the inline validity check, with an error message
/// describing the namespace location of the dereferenced object on its
/// owning UsdStage.
///
class UsdObject {
    typedef const TfToken UsdObject::*_UnspecifiedBoolType;

public:
    /// Default constructor produces an invalid object.
    UsdObject() : _type(UsdTypeObject) {}

    // --------------------------------------------------------------------- //
    /// \name Structural and Integrity Info about the Object itself
    /// @{
    // --------------------------------------------------------------------- //

    /// Return true if this is a valid object, false otherwise.
    bool IsValid() const {
        if (not UsdIsConcrete(_type) or not _prim)
            return false;
        if (_type == UsdTypePrim)
            return true;
        SdfSpecType specType = _GetDefiningSpecType();
        return (_type == UsdTypeAttribute and
                specType == SdfSpecTypeAttribute) or
            (_type == UsdTypeRelationship and
             specType == SdfSpecTypeRelationship);
    }

#ifdef doxygen
    /// Safe bool-conversion operator.  Equivalent to IsValid().
    operator unspecified-bool-type() const();
#else
    operator _UnspecifiedBoolType() const {
        return IsValid() ? &UsdObject::_propName : NULL;
    }
#endif // doxygen

public:

    /// Equality comparison.  Return true if \a lhs and \a rhs represent the
    /// same UsdObject, false otherwise.
    friend bool operator==(const UsdObject &lhs, const UsdObject &rhs) {
        return lhs._type == rhs._type and
            lhs._prim == rhs._prim and
            lhs._propName == rhs._propName;
    }

    /// Inequality comparison. Return false if \a lhs and \a rhs represent the
    /// same UsdObject, true otherwise.
    friend bool operator!=(const UsdObject &lhs, const UsdObject &rhs) {
        return not (lhs == rhs);
    }

    // hash_value overload for std/boost hash.
    friend size_t hash_value(const UsdObject &obj);

    /// Return the stage that owns the object, and to whose state and lifetime
    /// this object's validity is tied.
    UsdStageWeakPtr GetStage() const;

    /// Return the complete scene path to this object on its UsdStage,
    /// which may (UsdPrim) or may not (all other subclasses) return a 
    /// cached result
    SdfPath GetPath() const {
        // Allow getting expired object paths.
        if (Usd_PrimDataConstPtr p = get_pointer(_prim)) {
            return _type == UsdTypePrim ?
                p->GetPath() : p->GetPath().AppendProperty(_propName);
        }
        return SdfPath();
    }

    /// Return this object's path if this object is a prim, otherwise this
    /// object's nearest owning prim's path.  Equivalent to GetPrim().GetPath().
    const SdfPath &GetPrimPath() const {
        // Allow getting expired object paths.
        if (Usd_PrimDataConstPtr p = get_pointer(_prim))
            return p->GetPath();
        return SdfPath::EmptyPath();
    }

    /// Return this object if it is a prim, otherwise return this object's
    /// nearest owning prim.
    inline UsdPrim GetPrim() const;

    /// Return the full name of this object, i.e. the last component of its
    /// SdfPath in namespace.
    ///
    /// This is equivalent to, but generally cheaper than,
    /// GetPath().GetNameToken()
    const TfToken &GetName() const {
        return _type == UsdTypePrim ?
            _prim->GetPath().GetNameToken() : _propName;
    }

    /// Convert this UsdObject to another object type \p T if possible.  Return
    /// an invalid \p T instance if this object's dynamic type is not
    /// convertible to \p T or if this object is invalid.
    template <class T>
    T As() const {
        // compile-time type assertion provided by invoking Is<T>().
        return Is<T>() ? T(_type, _prim, _propName) : T();
    }

    /// Return true if this object is convertible to \p T.  This is equivalent
    /// to but cheaper than:
    /// \code
    /// bool(obj.As<T>())
    /// \endcode
    template <class T>
    bool Is() const {
        BOOST_MPL_ASSERT_MSG((boost::is_base_of<UsdObject, T>::value),
                             Provided_type_must_derive_or_be_UsdObject,
                             (T));
        return UsdIsConvertible(_type, _Detail::GetObjType<T>::Value);
    }

    /// Return a string that provides a brief summary description of the
    /// object.  This method, along with IsValid()/bool_operator,
    /// is always safe to call on a possibly-expired object, and the 
    /// description will specify whether the object is valid or expired,
    /// along with a few other bits of data.
    std::string GetDescription() const;
    
    // --------------------------------------------------------------------- //
    /// @}
    // --------------------------------------------------------------------- //


    // --------------------------------------------------------------------- //
    /// \name Generic Metadata Access
    /// @{
    // --------------------------------------------------------------------- //

    /// Resolve the requested metadatum named \p key into \p value,
    /// returning true on success.
    ///
    /// \return false if \p key was not resolvable, or if \p value's
    /// type \c T differed from that of the resolved metadatum.
    ///
    /// \note For any composition-related metadata, as enumerated in
    /// GetAllMetadata(), this method will return only the strongest
    /// opinion found, not applying the composition rules used by Pcp
    /// to process the data.  For more processed/composed views of
    /// composition data, please refer to the specific interface classes,
    /// such as UsdReferences, UsdInherits, UsdVariantSets, etc.
    ///
    /// \sa \ref Usd_OM_Metadata
    template<typename T>
    bool GetMetadata(const TfToken& key, T* value) const;
    /// \overload
    ///
    /// Type-erased access
    bool GetMetadata(const TfToken& key, VtValue* value) const;

    /// Set metadatum \p key's value to \p value.
    ///
    /// \return false if \p value's type does not match the schema type
    /// for \p key.
    ///
    /// \sa \ref Usd_OM_Metadata
    template<typename T>
    bool SetMetadata(const TfToken& key, const T& value) const;
    /// \overload
    bool SetMetadata(const TfToken& key, const VtValue& value) const;

    /// \brief Clears the authored \a key's value at the current EditTarget,
    /// returning false on error.
    ///
    /// If no value is present, this method is a no-op and returns true. It is
    /// considered an error to call ClearMetadata when no spec is present for 
    /// this UsdObject, i.e. if the object has no presence in the
    /// current UsdEditTarget.
    ///
    /// \sa \ref Usd_OM_Metadata
    bool ClearMetadata(const TfToken& key) const;

    /// \brief Returns true if the \a key has a meaningful value, that is, if
    /// GetMetadata() will provide a value, either because it was authored
    /// or because a prim's metadata fallback will be provided.
    bool HasMetadata(const TfToken& key) const;

    /// \brief Returns true if the \a key has an authored value, false if no
    /// value was authored or the only value available is a prim's metadata 
    /// fallback.
    bool HasAuthoredMetadata(const TfToken& key) const;

    /// \brief Resolve the requested dictionary sub-element \p keyPath of
    /// dictionary-valued metadatum named \p key into \p value,
    /// returning true on success.
    ///
    /// If you know you neeed just a small number of elements from a dictionary,
    /// accessing them element-wise using this method can be much less
    /// expensive than fetching the entire dictionary with GetMetadata(key).
    ///
    /// \return false if \p key was not resolvable, or if \p value's
    /// type \c T differed from that of the resolved metadatum.
    ///
    /// The \p keyPath is a ':'-separated path addressing an element
    /// in subdictionaries.
    ///
    /// \sa \ref Usd_Dictionary_Type
    template <class T>
    bool GetMetadataByDictKey(
        const TfToken& key, const TfToken &keyPath, T *value) const;
    /// \overload
    bool GetMetadataByDictKey(
        const TfToken& key, const TfToken &keyPath, VtValue *value) const;

    /// \brief Author \p value to the field identified by \p key and \p keyPath
    /// at the current EditTarget.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries stored in the metadata field at
    /// \p key.  Return true if the value is authored successfully, false
    /// otherwise.
    ///
    /// \sa \ref Usd_Dictionary_Type
    template<typename T>
    bool SetMetadataByDictKey(
        const TfToken& key, const TfToken &keyPath, const T& value) const;
    /// \overload
    bool SetMetadataByDictKey(
        const TfToken& key, const TfToken &keyPath, const VtValue& value) const;

    /// \brief Clear any authored value identified by \p key and \p keyPath
    /// at the current EditTarget.  The \p keyPath is a ':'-separated path
    /// identifying a path in subdictionaries stored in the metadata field at
    /// \p key.  Return true if the value is cleared successfully, false
    /// otherwise.
    ///
    /// \sa \ref Usd_Dictionary_Type
    bool ClearMetadataByDictKey(
        const TfToken& key, const TfToken& keyPath) const;

    /// \brief Return true if there exists any authored or fallback opinion for
    /// \p key and \p keyPath.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries stored in the metadata field at
    /// \p key.
    ///
    /// \sa \ref Usd_Dictionary_Type
    bool HasMetadataDictKey(
        const TfToken& key, const TfToken &keyPath) const;

    /// \brief Return true if there exists any authored opinion (excluding
    /// fallbacks) for \p key and \p keyPath.  The \p keyPath is a ':'-separated
    /// path identifying a value in subdictionaries stored in the metadata field
    /// at \p key.
    ///
    /// \sa \ref Usd_Dictionary_Type
    bool HasAuthoredMetadataDictKey(
        const TfToken& key, const TfToken &keyPath) const;

    /// \brief Resolve and return all metadata (including both authored and
    /// fallback values) on this object, sorted lexicographically.
    ///
    /// The keys returned in this map exactly match the keys returned by
    /// ListMetadata().
    ///
    /// \note This method does not return field keys for composition arcs,
    /// such as references, inherits, payloads, sublayers, variants, or
    /// primChildren, nor does it return the default value or timeSamples.
    UsdMetadataValueMap GetAllMetadata() const;

    /// \brief Resolve and return all user-authored metadata on this object,
    /// sorted lexicographically.
    ///
    /// This method returns a subset of the keys returned by ListMetadata.
    ///
    /// \note This method does not return field keys for composition arcs,
    /// such as references, inherits, payloads, sublayers, variants, or
    /// primChildren, nor does it return the default value or timeSamples.
    UsdMetadataValueMap GetAllAuthoredMetadata() const;

    // --------------------------------------------------------------------- //
    /// @}
    // --------------------------------------------------------------------- //

    // --------------------------------------------------------------------- //
    /// \name Core metadata fields
    /// @{
    // --------------------------------------------------------------------- //

    /// \brief Gets the value of the 'hidden' metadata field, false if not 
    /// authored.
    ///
    /// When an object is marked as hidden, it is an indicator to clients who 
    /// generically display objects (such as GUI widgets) that this object 
    /// should not be included, unless explicitly asked for.  Although this
    /// is just a hint and thus up to each application to interpret, we
    /// use it primarily as a way of simplifying hierarchy displays, by
    /// hiding \em only the representation of the object itself, \em not its
    /// subtree, instead "pulling up" everything below it one level in the
    /// hierarchical nesting.
    ///
    /// Note again that this is a hint for UI only - it should not be 
    /// interpreted by any renderer as making a prim invisible to drawing.
    bool IsHidden() const;

    /// \brief Sets the value of the 'hidden' metadata field. See IsHidden()
    /// for details.
    bool SetHidden(bool hidden) const;

    /// \brief Clears the opinion for "Hidden" at the current EditTarget.
    bool ClearHidden() const;

    /// \brief Returns true if hidden was explicitly authored and GetMetadata()
    /// will return a meaningful value for Hidden. 
    ///
    /// Note that IsHidden returns a fallback value (false) when hidden is not
    /// authored.
    bool HasAuthoredHidden() const;

    /// \brief Return this object's composed customData dictionary.
    ///
    /// CustomData is "custom metadata", a place for applications and users
    /// to put uniform data that is entirely dynamic and subject to no schema
    /// known to Usd.  Unlike metadata like 'hidden', 'displayName' etc,
    /// which must be declared in code or a data file that is considered part
    /// of one's Usd distribution (e.g. a plugInfo.json file) to be used,
    /// customData keys and the datatypes of their corresponding values are
    /// ad hoc.  No validation will ever be performed that values for the
    /// same key in different layers are of the same type - strongest simply
    /// wins.
    ///
    /// Dictionaries like customData are composed element-wise, and are 
    /// nestable.
    ///
    /// There is no means to query a customData field's valuetype other
    /// than fetching the value and interrogating it.
    /// \sa GetCustomDataByKey()
    VtDictionary GetCustomData() const;

    /// \brief Return the element identified by \p keyPath in this object's
    /// composed customData dictionary.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries.  This is in general more
    /// efficient than composing the entire customData dictionary and then
    /// pulling out one sub-element.
    VtValue GetCustomDataByKey(const TfToken &keyPath) const;

    /// \brief Author this object's customData dictionary to \p customData at
    /// the current EditTarget.
    void SetCustomData(const VtDictionary &customData) const;

    /// \brief Author the element identified by \p keyPath in this object's
    /// customData dictionary at the current EditTarget.  The \p keyPath is a
    /// ':'-separated path identifying a value in subdictionaries.
    void SetCustomDataByKey(const TfToken &keyPath, const VtValue &value) const;

    /// \brief Clear the authored opinion for this object's customData
    /// dictionary at the current EditTarget.  Do nothing if there is no such
    /// authored opinion.
    void ClearCustomData() const;

    /// \brief Clear the authored opinion identified by \p keyPath in this
    /// object's customData dictionary at the current EditTarget.  The \p
    /// keyPath is a ':'-separated path identifying a value in subdictionaries.
    /// Do nothing if there is no such authored opinion.
    void ClearCustomDataByKey(const TfToken &keyPath) const;

    /// \brief Return true if there are any authored or fallback opinions for
    /// this object's customData dictionary, false otherwise.
    bool HasCustomData() const;

    /// \brief Return true if there are any authored or fallback opinions for
    /// the element identified by \p keyPath in this object's customData
    /// dictionary, false otherwise.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries.
    bool HasCustomDataKey(const TfToken &keyPath) const;

    /// \brief Return true if there are any authored opinions (excluding
    /// fallback) for this object's customData dictionary, false otherwise.
    bool HasAuthoredCustomData() const;

    /// \brief Return true if there are any authored opinions (excluding
    /// fallback) for the element identified by \p keyPath in this object's
    /// customData dictionary, false otherwise.  The \p keyPath is a
    /// ':'-separated path identifying a value in subdictionaries.
    bool HasAuthoredCustomDataKey(const TfToken &keyPath) const;

    /// \brief Return this object's composed assetInfo dictionary.
    ///
    /// The asset info dictionary is used to annotate objects representing the 
    /// root-prims of assets (generally organized as models) with various 
    /// data related to asset management. For example, asset name, root layer
    /// identifier, asset version etc.
    ///
    /// The elements of this dictionary are composed element-wise, and are 
    /// nestable.
    ///
    /// There is no means to query an assetInfo field's valuetype other
    /// than fetching the value and interrogating it.
    /// \sa GetAssetInfoByKey()
    VtDictionary GetAssetInfo() const;

    /// \brief Return the element identified by \p keyPath in this object's
    /// composed assetInfo dictionary.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries.  This is in general more
    /// efficient than composing the entire assetInfo dictionary than 
    /// pulling out one sub-element.
    VtValue GetAssetInfoByKey(const TfToken &keyPath) const;

    /// \brief Author this object's assetInfo dictionary to \p assetInfo at
    /// the current EditTarget.
    void SetAssetInfo(const VtDictionary &customData) const;

    /// \brief Author the element identified by \p keyPath in this object's
    /// assetInfo dictionary at the current EditTarget.  The \p keyPath is a
    /// ':'-separated path identifying a value in subdictionaries.
    void SetAssetInfoByKey(const TfToken &keyPath, const VtValue &value) const;

    /// \brief Clear the authored opinion for this object's assetInfo
    /// dictionary at the current EditTarget.  Do nothing if there is no such
    /// authored opinion.
    void ClearAssetInfo() const;

    /// \brief Clear the authored opinion identified by \p keyPath in this
    /// object's assetInfo dictionary at the current EditTarget.  The \p
    /// keyPath is a ':'-separated path identifying a value in subdictionaries.
    /// Do nothing if there is no such authored opinion.
    void ClearAssetInfoByKey(const TfToken &keyPath) const;

    /// \brief Return true if there are any authored or fallback opinions for
    /// this object's assetInfo dictionary, false otherwise.
    bool HasAssetInfo() const;

    /// \brief Return true if there are any authored or fallback opinions for
    /// the element identified by \p keyPath in this object's assetInfo
    /// dictionary, false otherwise.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries.
    bool HasAssetInfoKey(const TfToken &keyPath) const;

    /// \brief Return true if there are any authored opinions (excluding
    /// fallback) for this object's assetInfo dictionary, false otherwise.
    bool HasAuthoredAssetInfo() const;

    /// \brief Return true if there are any authored opinions (excluding
    /// fallback) for the element identified by \p keyPath in this object's
    /// assetInfo dictionary, false otherwise.  The \p keyPath is a
    /// ':'-separated path identifying a value in subdictionaries.
    bool HasAuthoredAssetInfoKey(const TfToken &keyPath) const;

    /// Return this object's documentation (metadata).  This returns the
    /// empty string if no documentation has been set.
    /// \sa SetDocumentation()
    std::string GetDocumentation() const;

    /// Sets this object's documentation (metadata).  Returns true on success.
    bool SetDocumentation(const std::string& doc) const;

    /// Clears this object's documentation (metadata) in the current EditTarget
    /// (only).  Returns true on success.
    bool ClearDocumentation() const;

    /// Returns true if documentation was explicitly authored and GetMetadata()
    /// will return a meaningful value for documentation. 
    bool HasAuthoredDocumentation() const;

    // --------------------------------------------------------------------- //
    /// @}
    // --------------------------------------------------------------------- //

    // XXX: This method can and probably should move to UsdProperty
    static char GetNamespaceDelimiter() 
        { return SdfPathTokens->namespaceDelimiter.GetText()[0]; }
    
private:
    template <class T>
    bool _GetMetadataImpl(const TfToken& key,
                          T* value,
                          const TfToken &keyPath=TfToken()) const;
    template <class T>
    bool _SetMetadataImpl(const TfToken& key,
                          const T& value,
                          const TfToken &keyPath=TfToken()) const;
protected:
    // Private constructor for UsdPrim.
    UsdObject(const Usd_PrimDataHandle &prim)
        : _type(UsdTypePrim)
        , _prim(prim)
        {}

    // Private constructor for UsdAttribute/UsdRelationship.
    UsdObject(UsdObjType objType,
              const Usd_PrimDataHandle &prim,
              const TfToken &propName)
        : _type(objType)
        , _prim(prim)
        , _propName(propName) {}

    // Return the stage this object belongs to.
    UsdStage *_GetStage() const { return _prim->GetStage(); }

    // Return this object's defining spec type.
    SdfSpecType _GetDefiningSpecType() const;

    // Helper for subclasses: return held prim data.
    const Usd_PrimDataHandle &_Prim() const { return _prim; }

    // Helper for subclasses: return held propety name.
    const TfToken &_PropName() const { return _propName; }

private:
    // Helper for the above helper, and also for GetDescription()
    std::string _GetObjectDescription(const std::string &preface) const;

    friend class UsdStage;

    friend UsdObjType Usd_GetObjType(const UsdObject &obj) {
        return obj._type;
    }

    UsdObjType _type;
    Usd_PrimDataHandle _prim;
    TfToken _propName;

};


template<typename T>
bool
UsdObject::GetMetadata(const TfToken& key, T* value) const
{
    SdfAbstractDataTypedValue<T> result(value);
    return _GetMetadataImpl<SdfAbstractDataValue>(key, &result);
}

template<typename T>
bool 
UsdObject::SetMetadata(const TfToken& key, const T& value) const
{
    SdfAbstractDataConstTypedValue<T> in(&value);
    return _SetMetadataImpl<SdfAbstractDataConstValue>(key, in);
}

#endif //USD_OBJECT_H
