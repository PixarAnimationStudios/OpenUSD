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
#ifndef SDF_ABSTRACTDATA_H
#define SDF_ABSTRACTDATA_H

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/declarePtrs.h"

#include <boost/optional.hpp>
#include <vector>

TF_DECLARE_WEAK_AND_REF_PTRS(SdfAbstractData);
class SdfAbstractDataSpecVisitor;
class SdfAbstractDataConstValue;
class SdfAbstractDataValue;

#define SDF_DATA_TOKENS                  \
        ((TimeSamples, "timeSamples"))

TF_DECLARE_PUBLIC_TOKENS(SdfDataTokens, SDF_DATA_TOKENS);

/// \class SdfAbstractDataSpecId
///
/// Identifies a spec in an SdfAbstractData container. Conceptually, this
/// identifier is simply the spec's scene description path. However, this
/// object allows that identifier to be constructed in a variety of ways, 
/// potentially allowing the consumer to avoid path manipulations.
///
/// For efficiency, SdfAbstractDataSpecId objects do not make copies of
/// the path and token parameters passed to its constructors -- it just
/// holds on to pointers to them. The constructors take their parameters
/// by pointer to ensure that consumers do not pass in temporary values,
/// which could lead to undefined behavior.
class SdfAbstractDataSpecId
{
public:
    /// Construct an identifier for the spec at \p fullSpecPath.
    /// For convenience, this is intended to be implicitly constructible
    /// from an SdfPath.
    explicit SdfAbstractDataSpecId(const SdfPath* fullSpecPath)
        : _path(fullSpecPath)
        , _propertyName(NULL)
    {
    }

    /// Construct an identifier for the property named \p propertyName owned
    /// by the spec at \p owningSpecPath. If \p propertyName is empty, this
    /// constructs an identifier for the spec at \p owningSpecPath.
    SdfAbstractDataSpecId(const SdfPath* owningSpecPath, 
                          const TfToken* propertyName)
        : _path(owningSpecPath)
        , _propertyName(propertyName->IsEmpty() ? NULL : propertyName)
    {
    }

    /// Returns string representation of this key. Equivalent to
    /// GetFullSpecPath().GetString().
    std::string GetString() const;

    /// Returns true if this object identifies a property spec, false otherwise.
    bool IsProperty() const;

    /// Returns the full path to the spec identified by this object.
    const SdfPath& GetFullSpecPath() const
    {
        return (not _propertyName ? *_path : _ComputeFullSpecPath());
    }

    /// If this object identifies a property, returns the path to the spec
    /// that owns the property. Otherwise, returns the full path to the spec
    /// identified by this object.
    ///
    /// This path and the property name together form the full spec path.
    const SdfPath& GetPropertyOwningSpecPath() const
    {
        return (_propertyName or not _path->IsPropertyPath() ? 
            *_path : _ComputePropertyOwningSpecPath());
    }

    /// If this object identifies a property, returns that property's name.
    /// Otherwise, returns an empty token.
    ///
    /// The property-owning spec path and this name together form the full spec
    /// path.
    const TfToken& GetPropertyName() const;

private:
    const SdfPath& _ComputeFullSpecPath() const;
    const SdfPath& _ComputePropertyOwningSpecPath() const;

private:
    const SdfPath* _path;
    const TfToken* _propertyName;

    mutable boost::optional<SdfPath> _fullSpecPathBuffer;
    mutable boost::optional<SdfPath> _propertySpecPathBuffer;
};

/// \class SdfAbstractData
/// \brief Interface for scene description data storage.
///
/// This is not a layer.  SdfAbstractData is an anonymous container holding
/// scene description values.  It is like an STL container, but specialized
/// for holding scene description.
///
/// For any given SdfPath, an SdfAbstractData can hold one or more key/value 
/// pairs which we call Fields. Most of the API on SdfAbstractData accesses 
/// or modifies the value stored in a Field for a particular path and field 
/// name.
///
/// SdfAbstractData does not provide undo, change notification, or any strong
/// consistency guarantees about the scene description it contains.
/// Instead, it is a basis for building those things.
///
class SdfAbstractData : public TfRefBase, public TfWeakBase
{
public:
    SdfAbstractData() {}
    virtual ~SdfAbstractData(); 

    /// Copy the data in \p source into this data object.
    ///
    /// The default implementation does a spec-by-spec, field-by-field
    /// copy of \p source into this object.
    virtual void CopyFrom(const SdfAbstractDataConstPtr& source);

    /// Returns true if this data object has no specs, false otherwise.
    ///
    /// The default implementation uses a visitor to check if any specs
    /// exist.
    virtual bool IsEmpty() const;

    /// Returns true if this data object contains the same specs and fields
    /// as \a lhs, false otherwise. 
    ///
    /// The default implementation does a spec-by-spec, field-by-field
    /// comparison.
    // XXX: What are the right semmantics for this? 
    //      Does it matter if the underlying implementation matches?
    virtual bool Equals(const SdfAbstractDataRefPtr &rhs) const;

    /// Writes the contents of this data object to \p out. This is primarily
    /// for debugging purposes.
    ///
    /// The default implementation writes out each field for each spec.
    virtual void WriteToStream(std::ostream& out) const;

    /// \name Spec API
    /// @{

    /// Create a new spec at \a id with the given \a specType. If the spec
    /// already exists the spec type will be changed.
    virtual void CreateSpec(const SdfAbstractDataSpecId &id, 
                            SdfSpecType specType) = 0;

    /// Return true if this data has a spec for \a id.
    virtual bool HasSpec(const SdfAbstractDataSpecId &id) const = 0;

    /// Erase the spec at \a id and any fields that are on it.
    /// Note that this does not erase child specs.
    virtual void EraseSpec(const SdfAbstractDataSpecId &id) = 0;

    /// Move the spec at \a oldId to \a newId, including all the
    /// fields that are on it. This does not move any child specs.
    virtual void MoveSpec(const SdfAbstractDataSpecId &oldId, 
                          const SdfAbstractDataSpecId &newId) = 0;

    /// Return the spec type for the spec at \a id. Returns SdfSpecTypeUnknown
    /// if the spec doesn't exist.
    virtual SdfSpecType GetSpecType(const SdfAbstractDataSpecId &id) const = 0;

    /// Visits every spec in this SdfAbstractData object with the given 
    /// \p visitor. The order in which specs are visited is undefined. 
    /// The visitor may not modify the SdfAbstractData object it is visiting.
    /// \sa SdfAbstractDataSpecVisitor
    void VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

    /// @}

    /// \name Field API
    /// @{

    /// Returns whether a value exists for the given \a id and \a fieldName.
    /// Optionally returns the value if it exists.
    virtual bool Has(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                     SdfAbstractDataValue* value) const = 0;

    /// Return whether a value exists for the given \a id and \a fieldName.
    /// Optionally returns the value if it exists.
    virtual bool Has(const SdfAbstractDataSpecId& id, const TfToken &fieldName,
                     VtValue *value = NULL) const = 0;

    /// Return the value for the given \a id and \a fieldName. Returns an
    /// empty value if none is set.
    virtual VtValue Get(const SdfAbstractDataSpecId& id, 
                        const TfToken& fieldName) const = 0;

    /// Set the value of the given \a id and \a fieldName.
    ///
    /// It's an error to set a field on a spec that does not exist. Setting a
    /// field to an empty VtValue is the same as calling Erase() on it.
    virtual void Set(const SdfAbstractDataSpecId &id, const TfToken &fieldName,
                     const VtValue &value) = 0;

    /// Set the value of the given \a id and \a fieldName.
    ///
    /// It's an error to set a field on a spec that does not exist.
    virtual void Set(const SdfAbstractDataSpecId &id, const TfToken &fieldName,
                     const SdfAbstractDataConstValue& value) = 0;

    /// Remove the field at \p id and \p fieldName, if one exists.
    virtual void Erase(const SdfAbstractDataSpecId& id, 
                       const TfToken& fieldName) = 0;

    /// Return the names of all the fields that are set at \p id.
    virtual std::vector<TfToken> List(const SdfAbstractDataSpecId& id) const = 0;

    /// Return the value for the given \a id and \a fieldName. Returns the
    /// provided \a defaultValue value if none is set.
    template <class T>
    inline T GetAs(const SdfAbstractDataSpecId& id, const TfToken& fieldName,
                   const T& defaultValue = T()) const;

    /// @}


    /// \name Dict key access API
    /// @{

    // Return true and set \p value (if non null) if the field identified by
    // \p id and \p fieldName is dictionary-valued, and if there is an element
    // at \p keyPath in that dictionary.  Return false otherwise.  If
    // \p keyPath names an entire sub-dictionary, set \p value to that entire
    // sub-dictionary and return true.
    virtual bool HasDictKey(const SdfAbstractDataSpecId& id,
                            const TfToken &fieldName,
                            const TfToken &keyPath,
                            SdfAbstractDataValue* value) const;
    virtual bool HasDictKey(const SdfAbstractDataSpecId& id,
                            const TfToken &fieldName,
                            const TfToken &keyPath,
                            VtValue *value = NULL) const;

    // Same as HasDictKey but return empty VtValue on failure.
    virtual VtValue GetDictValueByKey(const SdfAbstractDataSpecId& id,
                                      const TfToken &fieldName,
                                      const TfToken &keyPath) const;

    // Set the element at \p keyPath in the dictionary-valued field identified
    // by \p id and \p fieldName.  If the field itself is not dictionary-valued,
    // replace the field with a new dictionary and set the element at \p keyPath
    // in it.  If \p value is empty, invoke EraseDictValueByKey instead.
    virtual void SetDictValueByKey(const SdfAbstractDataSpecId& id,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath,
                                   const VtValue &value);
    virtual void SetDictValueByKey(const SdfAbstractDataSpecId& id,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath,
                                   const SdfAbstractDataConstValue& value);

    // If \p id and \p fieldName identify a dictionary-valued field with an
    // element at \p keyPath, remove that element from the dictionary.  If this
    // leaves the dictionary empty, Erase() the entire field.
    virtual void EraseDictValueByKey(const SdfAbstractDataSpecId& id,
                                     const TfToken &fieldName,
                                     const TfToken &keyPath);

    // If \p id, \p fieldName, and \p keyPath identify a (sub) dictionary,
    // return a vector of the keys in that dictionary, otherwise return an empty
    // vector.
    virtual std::vector<TfToken> ListDictKeys(const SdfAbstractDataSpecId& id,
                                              const TfToken &fieldName,
                                              const TfToken &keyPath) const;


    /// @}


    /// \name Time-sample API
    ///
    /// This API supports narrowly-targeted queries against the
    /// "timeSamples" key of properties.  In particular, it enables
    /// asking for single time samples without pulling on the entire
    /// set of time samples, as well as asking about the set of sample
    /// times without pulling on the actual values at those times.
    ///
    /// @{

    virtual std::set<double>
    ListAllTimeSamples() const = 0;
    
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfAbstractDataSpecId& id) const = 0;

    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const = 0;

    virtual size_t
    GetNumTimeSamplesForPath(const SdfAbstractDataSpecId& id) const = 0;

    virtual bool
    GetBracketingTimeSamplesForPath(const SdfAbstractDataSpecId& id, 
                                    double time,
                                    double* tLower, double* tUpper) const = 0;

    virtual bool
    QueryTimeSample(const SdfAbstractDataSpecId& id, double time,
                    VtValue *optionalValue = NULL) const = 0;
    virtual bool
    QueryTimeSample(const SdfAbstractDataSpecId& id, double time,
                    SdfAbstractDataValue *optionalValue) const = 0;

    virtual void
    SetTimeSample(const SdfAbstractDataSpecId& id, double time, 
                  const VtValue & value) = 0;

    virtual void
    EraseTimeSample(const SdfAbstractDataSpecId& id, double time) = 0;

    /// @}

protected:
    /// Visits every spec in this SdfAbstractData object with the given 
    /// \p visitor. The order in which specs are visited is undefined. 
    /// The visitor may not modify the SdfAbstractData object it is visiting.
    /// This method should \b not call \c Done() on the visitor.
    /// \sa SdfAbstractDataSpecVisitor
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const = 0;
};

template <class T>
inline T SdfAbstractData::GetAs(
    const SdfAbstractDataSpecId& id, 
    const TfToken& field, const T& defaultVal) const
{
    VtValue val = Get(id, field);
    if (val.IsHolding<T>()) {
        return val.UncheckedGet<T>();
    }
    return defaultVal;
}

/// \class SdfAbstractDataValue
/// A type-erased container for a field value in an SdfAbstractData.
/// See SdfAbstractDataTypedValue for more details.
///
/// \sa SdfAbstractDataTypedValue
class SdfAbstractDataValue
{
public:
    virtual bool StoreValue(const VtValue& value) = 0;

    template <class T> bool StoreValue(const T& v) 
    {
        if (TfSafeTypeCompare(typeid(T), valueType)) {
            *static_cast<T*>(value) = v;
            return true;
        }
        return false;
    }

    void* value;
    const std::type_info& valueType;

protected:
    SdfAbstractDataValue(void* value_, const std::type_info& valueType_)
        : value(value_)
        , valueType(valueType_)
    { }
};

/// \class SdfAbstractDataTypedValue
///
/// The fully-typed container for a field value in an \c SdfAbstractData.
/// An \c SdfAbstractDataTypedValue allows a consumer to pass a pointer to
/// an object through the virtual \c SdfAbstractData interface along with
/// information about that object's type. That information may allow
/// implementations of \c SdfAbstractData to populate the contained object
/// in a more efficient way, avoiding unnecessary boxing/unboxing of data.
///
/// SdfAbstractDataTypedValue objects are intended to be transient; they
/// are solely used to get pointer information into and out of an 
/// SdfAbstractData container.
template <class T>
class SdfAbstractDataTypedValue : public SdfAbstractDataValue
{
public:
    SdfAbstractDataTypedValue(T* value)
        : SdfAbstractDataValue(value, typeid(T))
    { }

    virtual bool StoreValue(const VtValue& v)
    {
        if (not v.IsHolding<T>()) {
            return false;
        }

        *static_cast<T*>(value) = v.UncheckedGet<T>();
        return true;
    }
};

/// \class SdfAbstractDataConstValue
/// A type-erased container for a const field value in an SdfAbstractData.
/// See SdfAbstractDataConstTypedValue for more details.
///
/// \sa SdfAbstractDataConstTypedValue
class SdfAbstractDataConstValue
{
public:
    virtual bool GetValue(VtValue* value) const = 0;

    template <class T> bool GetValue(T* v) const
    {
        if (TfSafeTypeCompare(typeid(T), valueType)) {
            *v = *static_cast<const T*>(value);
            return true;
        }
        return false;
    }

    virtual bool IsEqual(const VtValue& value) const = 0;

    const void* value;
    const std::type_info& valueType;

protected:
    SdfAbstractDataConstValue(const void* value_, 
                             const std::type_info& valueType_)
        : value(value_)
        , valueType(valueType_)
    { }
};

/// \class SdfAbstractDataConstTypedValue
///
/// The fully-typed container for a field value in an \c SdfAbstractData.
/// An \c SdfAbstractDataConstTypedValue allows a consumer to pass a pointer to
/// an object through the virtual \c SdfAbstractData interface along with
/// information about that object's type. That information may allow
/// implementations of \c SdfAbstractData to store the contained object
/// in a more efficient way, avoiding unnecessary boxing/unboxing of data.
///
/// SdfAbstractDataConstTypedValue objects are intended to be transient; they
/// are solely used to get pointer information into an SdfAbstractData 
/// container.
template <class T>
class SdfAbstractDataConstTypedValue : public SdfAbstractDataConstValue
{
public:
    SdfAbstractDataConstTypedValue(const T* value)
        : SdfAbstractDataConstValue(value, typeid(T))
    { }

    virtual bool GetValue(VtValue* v) const
    {
        *v = _GetValue();
        return true;
    }

    virtual bool IsEqual(const VtValue& v) const
    {
        return (v.IsHolding<T>() and v.UncheckedGet<T>() == _GetValue());
    }

private:
    const T& _GetValue() const 
    {
        return *static_cast<const T*>(value);
    }
};

// Specialization of SdAbstractDataConstTypedValue that converts character
// arrays to std::string. 
template <int N>
class SdfAbstractDataConstTypedValue<char[N]> 
    : public SdfAbstractDataConstTypedValue<std::string>
{
public:
    typedef char CharArray[N];
    SdfAbstractDataConstTypedValue(const CharArray* value)
        : SdfAbstractDataConstTypedValue<std::string>(&_str)
        , _str(*value)
    { }

private:
    std::string _str;
};

/// \class SdfAbstractDataSpecVisitor
/// Base class for objects used to visit specs in an SdfAbstractData object.
/// \sa SdfAbstractData::VisitSpecs.
class SdfAbstractDataSpecVisitor
{
public:
    virtual ~SdfAbstractDataSpecVisitor();

    /// \c SdfAbstractData::VisitSpecs will call this function for every entry
    /// it contains, passing itself as \p data and the entry's spec id as \p id.
    /// If this function returns false, the iteration through the entries 
    /// will end early, otherwise it will continue.
    virtual bool VisitSpec(const SdfAbstractData& data, 
                           const SdfAbstractDataSpecId& id) = 0;

    /// \c SdfAbstractData::VisitSpecs will call this after visitation is
    /// complete, even if some \c VisitSpec() returned \c false.
    virtual void Done(const SdfAbstractData& data) = 0;
};

#endif // SD_ABSTRACTDATA_H
