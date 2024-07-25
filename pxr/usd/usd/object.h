//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_OBJECT_H
#define PXR_USD_USD_OBJECT_H

/// \file usd/object.h

#include "pxr/pxr.h"
#include "pxr/usd/usd/api.h"
#include "pxr/usd/usd/common.h"
#include "pxr/usd/usd/primData.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/sdf/abstractData.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/hash.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_PTRS(UsdStage);

/// \enum UsdObjType
///
/// Enum values to represent the various Usd object types.
///
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
    static_assert(std::is_base_of<UsdObject, T>::value,
                  "Type T must be a subclass of UsdObject.");
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
    return (baseType == UsdTypeObject) || (baseType == subType) ||
        (baseType == UsdTypeProperty && subType > UsdTypeProperty);
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
    return type == UsdTypePrim   ||
        type == UsdTypeAttribute ||
        type == UsdTypeRelationship;
}

/// \class UsdObject
///
/// Base class for Usd scenegraph objects, providing common API.
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
/// validity through an \em explicit-bool operator, so client code should always 
/// be able use objects safely, even across edits to the owning UsdStage. 
/// UsdObject classes also perform some level of validity checking upon every 
/// use, in order to facilitate debugging of unsafe code, although we reserve 
/// the right to activate that behavior only in debug builds, if it becomes 
/// compelling to do so for performance reasons.  This per-use checking will 
/// cause a fatal error upon failing the inline validity check, with an error 
/// message describing the namespace location of the dereferenced object on its
/// owning UsdStage.
///
class UsdObject {
public:
    /// Default constructor produces an invalid object.
    UsdObject() : _type(UsdTypeObject) {}

    // --------------------------------------------------------------------- //
    /// \name Structural and Integrity Info about the Object itself
    /// @{
    // --------------------------------------------------------------------- //

    /// Return true if this is a valid object, false otherwise.
    bool IsValid() const {
        if (!UsdIsConcrete(_type) || !_prim)
            return false;
        if (_type == UsdTypePrim)
            return true;
        SdfSpecType specType = _GetDefiningSpecType();
        return (_type == UsdTypeAttribute &&
                specType == SdfSpecTypeAttribute) ||
            (_type == UsdTypeRelationship &&
             specType == SdfSpecTypeRelationship);
    }

    /// Returns \c true if this object is valid, \c false otherwise.
    explicit operator bool() const {
        return IsValid();
    }

public:

    /// Equality comparison.  Return true if \a lhs and \a rhs represent the
    /// same UsdObject, false otherwise.
    friend bool operator==(const UsdObject &lhs, const UsdObject &rhs) {
        return lhs._type == rhs._type &&
            lhs._prim == rhs._prim &&
            lhs._proxyPrimPath == rhs._proxyPrimPath &&
            lhs._propName == rhs._propName;
    }

    /// Inequality comparison. Return false if \a lhs and \a rhs represent the
    /// same UsdObject, true otherwise.
    friend bool operator!=(const UsdObject &lhs, const UsdObject &rhs) {
        return !(lhs == rhs);
    }

    /// Less-than operator. Returns true if \a lhs < \a rhs. 
    /// 
    /// This simply compares the paths of the objects. 
    friend bool operator<(const UsdObject &lhs, const UsdObject &rhs) {
        return lhs.GetPath() < rhs.GetPath();
    }

    // hash_value overload for std/boost hash.
    friend size_t hash_value(const UsdObject &obj) {
        return TfHash()(obj);
    }

    // TfHash support
    template <class HashState>
    friend void TfHashAppend(HashState &h, const UsdObject &obj) {
        h.Append(obj._type, obj._prim, obj._proxyPrimPath, obj._propName);
    }

    /// Return the stage that owns the object, and to whose state and lifetime
    /// this object's validity is tied.
    USD_API
    UsdStageWeakPtr GetStage() const;

    /// Return the complete scene path to this object on its UsdStage,
    /// which may (UsdPrim) or may not (all other subclasses) return a 
    /// cached result
    SdfPath GetPath() const {
        // Allow getting expired object paths.
        if (!_proxyPrimPath.IsEmpty()) { 
            return _type == UsdTypePrim ?
                _proxyPrimPath : _proxyPrimPath.AppendProperty(_propName);
        }
        else if (Usd_PrimDataConstPtr p = get_pointer(_prim)) {
            return _type == UsdTypePrim ?
                p->GetPath() : p->GetPath().AppendProperty(_propName);
        }
        return SdfPath();
    }

    /// Return this object's path if this object is a prim, otherwise this
    /// object's nearest owning prim's path.  Equivalent to GetPrim().GetPath().
    const SdfPath &GetPrimPath() const {
        // Allow getting expired object paths.
        if (!_proxyPrimPath.IsEmpty()) {
            return _proxyPrimPath;
        }
        else if (Usd_PrimDataConstPtr p = get_pointer(_prim)) {
            return p->GetPath();
        }
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
        return _type == UsdTypePrim ? GetPrimPath().GetNameToken() : _propName;
    }

    /// Convert this UsdObject to another object type \p T if possible.  Return
    /// an invalid \p T instance if this object's dynamic type is not
    /// convertible to \p T or if this object is invalid.
    template <class T>
    T As() const {
        // compile-time type assertion provided by invoking Is<T>().
        return Is<T>() ? T(_type, _prim, _proxyPrimPath, _propName) : T();
    }

    /// Return true if this object is convertible to \p T.  This is equivalent
    /// to but cheaper than:
    /// \code
    /// bool(obj.As<T>())
    /// \endcode
    template <class T>
    bool Is() const {
        static_assert(std::is_base_of<UsdObject, T>::value,
                      "Provided type T must derive from or be UsdObject");
        return UsdIsConvertible(_type, _Detail::GetObjType<T>::Value);
    }

    /// Return a string that provides a brief summary description of the
    /// object.  This method, along with IsValid()/bool_operator,
    /// is always safe to call on a possibly-expired object, and the 
    /// description will specify whether the object is valid or expired,
    /// along with a few other bits of data.
    USD_API
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
    USD_API
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
    USD_API
    bool SetMetadata(const TfToken& key, const VtValue& value) const;

    /// Clears the authored \a key's value at the current EditTarget,
    /// returning false on error.
    ///
    /// If no value is present, this method is a no-op and returns true. It is
    /// considered an error to call ClearMetadata when no spec is present for 
    /// this UsdObject, i.e. if the object has no presence in the
    /// current UsdEditTarget.
    ///
    /// \sa \ref Usd_OM_Metadata
    USD_API
    bool ClearMetadata(const TfToken& key) const;

    /// Returns true if the \a key has a meaningful value, that is, if
    /// GetMetadata() will provide a value, either because it was authored
    /// or because a prim's metadata fallback will be provided.
    USD_API
    bool HasMetadata(const TfToken& key) const;

    /// Returns true if the \a key has an authored value, false if no
    /// value was authored or the only value available is a prim's metadata 
    /// fallback.
    USD_API
    bool HasAuthoredMetadata(const TfToken& key) const;

    /// Resolve the requested dictionary sub-element \p keyPath of
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
    USD_API
    bool GetMetadataByDictKey(
        const TfToken& key, const TfToken &keyPath, VtValue *value) const;

    /// Author \p value to the field identified by \p key and \p keyPath
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
    USD_API
    bool SetMetadataByDictKey(
        const TfToken& key, const TfToken &keyPath, const VtValue& value) const;

    /// Clear any authored value identified by \p key and \p keyPath
    /// at the current EditTarget.  The \p keyPath is a ':'-separated path
    /// identifying a path in subdictionaries stored in the metadata field at
    /// \p key.  Return true if the value is cleared successfully, false
    /// otherwise.
    ///
    /// \sa \ref Usd_Dictionary_Type
    USD_API
    bool ClearMetadataByDictKey(
        const TfToken& key, const TfToken& keyPath) const;

    /// Return true if there exists any authored or fallback opinion for
    /// \p key and \p keyPath.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries stored in the metadata field at
    /// \p key.
    ///
    /// \sa \ref Usd_Dictionary_Type
    USD_API
    bool HasMetadataDictKey(
        const TfToken& key, const TfToken &keyPath) const;

    /// Return true if there exists any authored opinion (excluding
    /// fallbacks) for \p key and \p keyPath.  The \p keyPath is a ':'-separated
    /// path identifying a value in subdictionaries stored in the metadata field
    /// at \p key.
    ///
    /// \sa \ref Usd_Dictionary_Type
    USD_API
    bool HasAuthoredMetadataDictKey(
        const TfToken& key, const TfToken &keyPath) const;

    /// Resolve and return all metadata (including both authored and
    /// fallback values) on this object, sorted lexicographically.
    ///
    /// \note This method does not return field keys for composition arcs, such
    /// as references, inherits, payloads, sublayers, variants, or primChildren,
    /// nor does it return the default value, timeSamples, or spline.
    USD_API
    UsdMetadataValueMap GetAllMetadata() const;

    /// Resolve and return all user-authored metadata on this object,
    /// sorted lexicographically.
    ///
    /// \note This method does not return field keys for composition arcs, such
    /// as references, inherits, payloads, sublayers, variants, or primChildren,
    /// nor does it return the default value, timeSamples, or spline.
    USD_API
    UsdMetadataValueMap GetAllAuthoredMetadata() const;

    // --------------------------------------------------------------------- //
    /// @}
    // --------------------------------------------------------------------- //

    // --------------------------------------------------------------------- //
    /// \name Core metadata fields
    /// @{
    // --------------------------------------------------------------------- //

    /// Gets the value of the 'hidden' metadata field, false if not 
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
    USD_API
    bool IsHidden() const;

    /// Sets the value of the 'hidden' metadata field. See IsHidden()
    /// for details.
    USD_API
    bool SetHidden(bool hidden) const;

    /// Clears the opinion for "Hidden" at the current EditTarget.
    USD_API
    bool ClearHidden() const;

    /// Returns true if hidden was explicitly authored and GetMetadata()
    /// will return a meaningful value for Hidden. 
    ///
    /// Note that IsHidden returns a fallback value (false) when hidden is not
    /// authored.
    USD_API
    bool HasAuthoredHidden() const;

    /// Return this object's composed customData dictionary.
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
    USD_API
    VtDictionary GetCustomData() const;

    /// Return the element identified by \p keyPath in this object's
    /// composed customData dictionary.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries.  This is in general more
    /// efficient than composing the entire customData dictionary and then
    /// pulling out one sub-element.
    USD_API
    VtValue GetCustomDataByKey(const TfToken &keyPath) const;

    /// Author this object's customData dictionary to \p customData at
    /// the current EditTarget.
    USD_API
    void SetCustomData(const VtDictionary &customData) const;

    /// Author the element identified by \p keyPath in this object's
    /// customData dictionary at the current EditTarget.  The \p keyPath is a
    /// ':'-separated path identifying a value in subdictionaries.
    USD_API
    void SetCustomDataByKey(const TfToken &keyPath, const VtValue &value) const;

    /// Clear the authored opinion for this object's customData
    /// dictionary at the current EditTarget.  Do nothing if there is no such
    /// authored opinion.
    USD_API
    void ClearCustomData() const;

    /// Clear the authored opinion identified by \p keyPath in this
    /// object's customData dictionary at the current EditTarget.  The \p
    /// keyPath is a ':'-separated path identifying a value in subdictionaries.
    /// Do nothing if there is no such authored opinion.
    USD_API
    void ClearCustomDataByKey(const TfToken &keyPath) const;

    /// Return true if there are any authored or fallback opinions for
    /// this object's customData dictionary, false otherwise.
    USD_API
    bool HasCustomData() const;

    /// Return true if there are any authored or fallback opinions for
    /// the element identified by \p keyPath in this object's customData
    /// dictionary, false otherwise.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries.
    USD_API
    bool HasCustomDataKey(const TfToken &keyPath) const;

    /// Return true if there are any authored opinions (excluding
    /// fallback) for this object's customData dictionary, false otherwise.
    USD_API
    bool HasAuthoredCustomData() const;

    /// Return true if there are any authored opinions (excluding
    /// fallback) for the element identified by \p keyPath in this object's
    /// customData dictionary, false otherwise.  The \p keyPath is a
    /// ':'-separated path identifying a value in subdictionaries.
    USD_API
    bool HasAuthoredCustomDataKey(const TfToken &keyPath) const;

    /// Return this object's composed assetInfo dictionary.
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
    USD_API
    VtDictionary GetAssetInfo() const;

    /// Return the element identified by \p keyPath in this object's
    /// composed assetInfo dictionary.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries.  This is in general more
    /// efficient than composing the entire assetInfo dictionary than 
    /// pulling out one sub-element.
    USD_API
    VtValue GetAssetInfoByKey(const TfToken &keyPath) const;

    /// Author this object's assetInfo dictionary to \p assetInfo at
    /// the current EditTarget.
    USD_API
    void SetAssetInfo(const VtDictionary &customData) const;

    /// Author the element identified by \p keyPath in this object's
    /// assetInfo dictionary at the current EditTarget.  The \p keyPath is a
    /// ':'-separated path identifying a value in subdictionaries.
    USD_API
    void SetAssetInfoByKey(const TfToken &keyPath, const VtValue &value) const;

    /// Clear the authored opinion for this object's assetInfo
    /// dictionary at the current EditTarget.  Do nothing if there is no such
    /// authored opinion.
    USD_API
    void ClearAssetInfo() const;

    /// Clear the authored opinion identified by \p keyPath in this
    /// object's assetInfo dictionary at the current EditTarget.  The \p
    /// keyPath is a ':'-separated path identifying a value in subdictionaries.
    /// Do nothing if there is no such authored opinion.
    USD_API
    void ClearAssetInfoByKey(const TfToken &keyPath) const;

    /// Return true if there are any authored or fallback opinions for
    /// this object's assetInfo dictionary, false otherwise.
    USD_API
    bool HasAssetInfo() const;

    /// Return true if there are any authored or fallback opinions for
    /// the element identified by \p keyPath in this object's assetInfo
    /// dictionary, false otherwise.  The \p keyPath is a ':'-separated path
    /// identifying a value in subdictionaries.
    USD_API
    bool HasAssetInfoKey(const TfToken &keyPath) const;

    /// Return true if there are any authored opinions (excluding
    /// fallback) for this object's assetInfo dictionary, false otherwise.
    USD_API
    bool HasAuthoredAssetInfo() const;

    /// Return true if there are any authored opinions (excluding
    /// fallback) for the element identified by \p keyPath in this object's
    /// assetInfo dictionary, false otherwise.  The \p keyPath is a
    /// ':'-separated path identifying a value in subdictionaries.
    USD_API
    bool HasAuthoredAssetInfoKey(const TfToken &keyPath) const;

    /// Return this object's documentation (metadata).  This returns the
    /// empty string if no documentation has been set.
    /// \sa SetDocumentation()
    USD_API
    std::string GetDocumentation() const;

    /// Sets this object's documentation (metadata).  Returns true on success.
    USD_API
    bool SetDocumentation(const std::string& doc) const;

    /// Clears this object's documentation (metadata) in the current EditTarget
    /// (only).  Returns true on success.
    USD_API
    bool ClearDocumentation() const;

    /// Returns true if documentation was explicitly authored and GetMetadata()
    /// will return a meaningful value for documentation. 
    USD_API
    bool HasAuthoredDocumentation() const;

    /// Return this object's display name (metadata).  This returns the
    /// empty string if no display name has been set.
    /// \sa SetDisplayName()
    USD_API
    std::string GetDisplayName() const;

    /// Sets this object's display name (metadata).  Returns true on success.
    ///
    /// DisplayName is meant to be a descriptive label, not necessarily an
    /// alternate identifier; therefore there is no restriction on which
    /// characters can appear in it.
    USD_API
    bool SetDisplayName(const std::string& name) const;

    /// Clears this object's display name (metadata) in the current EditTarget
    /// (only).  Returns true on success.
    USD_API
    bool ClearDisplayName() const;

    /// Returns true if displayName was explicitly authored and GetMetadata()
    /// will return a meaningful value for displayName. 
    USD_API
    bool HasAuthoredDisplayName() const;

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

    bool _GetMetadataImpl(const TfToken& key,
                          VtValue* value,
                          const TfToken &keyPath=TfToken()) const;

    template <class T>
    bool _SetMetadataImpl(const TfToken& key,
                          const T& value,
                          const TfToken &keyPath=TfToken()) const;

    bool _SetMetadataImpl(const TfToken& key,
                          const VtValue& value,
                          const TfToken &keyPath=TfToken()) const;

protected:
    template <class Derived> struct _Null {};

    // Private constructor for null dervied types.
    template <class Derived>
    explicit UsdObject(_Null<Derived>)
        : _type(_Detail::GetObjType<Derived>::Value) {}
    
    // Private constructor for UsdPrim.
    UsdObject(const Usd_PrimDataHandle &prim,
              const SdfPath &proxyPrimPath)
        : _type(UsdTypePrim)
        , _prim(prim)
        , _proxyPrimPath(proxyPrimPath)
    {
        TF_VERIFY(!_prim || _prim->GetPath() != _proxyPrimPath);
    }

    // Private constructor for UsdAttribute/UsdRelationship.
    UsdObject(UsdObjType objType,
              const Usd_PrimDataHandle &prim,
              const SdfPath &proxyPrimPath,
              const TfToken &propName)
        : _type(objType)
        , _prim(prim)
        , _proxyPrimPath(proxyPrimPath)
        , _propName(propName) 
    {
        TF_VERIFY(!_prim || _prim->GetPath() != _proxyPrimPath);
    }

    // Return the stage this object belongs to.
    UsdStage *_GetStage() const { return _prim->GetStage(); }

    // Return this object's defining spec type.
    USD_API
    SdfSpecType _GetDefiningSpecType() const;

    // Helper for subclasses: return held prim data.
    const Usd_PrimDataHandle &_Prim() const { return _prim; }

    // Helper for subclasses: return held property name.
    const TfToken &_PropName() const { return _propName; }

    // Helper for subclasses: return held proxy prim path.
    const SdfPath &_ProxyPrimPath() const { return _proxyPrimPath; }

private:
    // Helper for the above helper, and also for GetDescription()
    std::string _GetObjectDescription(const std::string &preface) const;

    friend class UsdStage;

    friend UsdObjType Usd_GetObjType(const UsdObject &obj) {
        return obj._type;
    }

    UsdObjType _type;
    Usd_PrimDataHandle _prim;
    SdfPath _proxyPrimPath;
    TfToken _propName;

};

template<typename T>
inline
bool
UsdObject::GetMetadata(const TfToken& key, T* value) const
{
    return _GetMetadataImpl(key, value);
}

template<typename T>
inline
bool 
UsdObject::SetMetadata(const TfToken& key, const T& value) const
{
    return _SetMetadataImpl(key, value);
}

template <typename T>
inline
bool
UsdObject::GetMetadataByDictKey(const TfToken& key, 
                                const TfToken &keyPath, 
                                T *value) const
{
    return _GetMetadataImpl(key, value, keyPath);
}

template <typename T>
inline
bool
UsdObject::SetMetadataByDictKey(const TfToken& key, 
                                const TfToken &keyPath, 
                                const T& value) const
{
    return _SetMetadataImpl(key, value, keyPath);
}

template <class T>
bool 
UsdObject::_GetMetadataImpl(const TfToken& key,
                            T* value,
                            const TfToken &keyPath) const
{
    return _GetStage()->_GetMetadata(
        *this, key, keyPath, /*useFallbacks=*/true, value);
}

template <class T>
bool 
UsdObject::_SetMetadataImpl(const TfToken& key,
                            const T& value,
                            const TfToken &keyPath) const
{
    return _GetStage()->_SetMetadata(*this, key, keyPath, value);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_USD_OBJECT_H
