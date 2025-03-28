//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_ABSTRACT_DATA_H
#define PXR_USD_SDF_ABSTRACT_DATA_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/vt/value.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/declarePtrs.h"

#include <vector>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(SdfAbstractData);
class SdfAbstractDataSpecVisitor;
class SdfAbstractDataConstValue;
class SdfAbstractDataValue;


#define SDF_DATA_TOKENS                  \
        ((TimeSamples, "timeSamples"))

TF_DECLARE_PUBLIC_TOKENS(SdfDataTokens, SDF_API, SDF_DATA_TOKENS);


/// \class SdfAbstractData
///
/// Interface for scene description data storage.
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
    SDF_API
    virtual ~SdfAbstractData(); 

    /// Copy the data in \p source into this data object.
    ///
    /// The default implementation does a spec-by-spec, field-by-field
    /// copy of \p source into this object.
    SDF_API
    virtual void CopyFrom(const SdfAbstractDataConstPtr& source);

    /// Returns true if this data object streams its data to and from its
    /// serialized data store on demand.
    ///
    /// Sdf will treat layers with streaming data differently to avoid pulling
    /// in data unnecessarily. For example, reloading a streaming layer 
    /// will not perform fine-grained change notification, since doing 
    /// so would require the full contents of the layer to be loaded.
    SDF_API
    virtual bool StreamsData() const = 0;

    /// Returns true if this data object is detached from its serialized
    /// data store, false otherwise. A detached data object must not be
    /// affected by external changes to the serialized data.
    ///
    /// Sdf allows clients to specify detached layers to avoid problems
    /// that may occur if the underlying data is modified by an external
    /// process. For example, a data object that maintains an open file
    /// handle or memory mapping to the original layer on disk and reads
    /// data on demand is not detached. But a data object that pulls all
    /// of the layer contents into memory is detached.
    ///
    /// The default implementation returns !StreamsData(). Non-streaming
    /// data objects are assumed to be detached from their serialized
    /// data, while streaming objects are conservatively assumed to
    /// not be detached. Note that it is possible to have a streaming
    /// data object that is also detached -- for example, if the data
    /// object were to make a private copy of the serialized data for
    /// its own use and streamed data from it.
    SDF_API
    virtual bool IsDetached() const;

    /// Returns true if this data object has no specs, false otherwise.
    ///
    /// The default implementation uses a visitor to check if any specs
    /// exist.
    SDF_API
    virtual bool IsEmpty() const;

    /// Returns true if this data object contains the same specs and fields
    /// as \a lhs, false otherwise. 
    ///
    /// The default implementation does a spec-by-spec, field-by-field
    /// comparison.
    // XXX: What are the right semantics for this?
    //      Does it matter if the underlying implementation matches?
    SDF_API
    virtual bool Equals(const SdfAbstractDataRefPtr &rhs) const;

    /// Writes the contents of this data object to \p out. This is primarily
    /// for debugging purposes.
    ///
    /// The default implementation writes out each field for each spec.
    SDF_API
    virtual void WriteToStream(std::ostream& out) const;

    /// \name Spec API
    /// @{

    /// Create a new spec at \a path with the given \a specType. If the spec
    /// already exists the spec type will be changed.
    SDF_API
    virtual void CreateSpec(const SdfPath &path, 
                            SdfSpecType specType) = 0;

    /// Return true if this data has a spec for \a path.
    SDF_API
    virtual bool HasSpec(const SdfPath &path) const = 0;

    /// Erase the spec at \a path and any fields that are on it.
    /// Note that this does not erase child specs.
    SDF_API
    virtual void EraseSpec(const SdfPath &path) = 0;

    /// Move the spec at \a oldPath to \a newPath, including all the
    /// fields that are on it. This does not move any child specs.
    SDF_API
    virtual void MoveSpec(const SdfPath &oldPath, 
                          const SdfPath &newPath) = 0;

    /// Return the spec type for the spec at \a path. Returns SdfSpecTypeUnknown
    /// if the spec doesn't exist.
    virtual SdfSpecType GetSpecType(const SdfPath &path) const = 0;

    /// Visits every spec in this SdfAbstractData object with the given 
    /// \p visitor. The order in which specs are visited is undefined. 
    /// The visitor may not modify the SdfAbstractData object it is visiting.
    /// \sa SdfAbstractDataSpecVisitor
    SDF_API
    void VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

    /// @}

    /// \name Field API
    /// @{

    /// Returns whether a value exists for the given \a path and \a fieldName.
    /// Optionally returns the value if it exists.
    SDF_API
    virtual bool Has(const SdfPath& path, const TfToken& fieldName,
                     SdfAbstractDataValue* value) const = 0;

    /// Return whether a value exists for the given \a path and \a fieldName.
    /// Optionally returns the value if it exists.
    SDF_API
    virtual bool Has(const SdfPath& path, const TfToken &fieldName,
                     VtValue *value = NULL) const = 0;

    /// Fill \p specType (which cannot be nullptr) as if by a call to
    /// GetSpecType(path).  If the resulting specType is not SdfSpecTypeUnknown,
    /// then act as if Has(path, fieldName, value) was called and return its
    /// result.  In other words, the semantics of this function must be
    /// identical to this sequence:
    ///
    /// \code
    /// *specType = GetSpecType(path);
    /// return *specType != SdfSpecTypeUnknown && Has(path, fieldName, value);
    /// \endcode
    SDF_API
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    SdfAbstractDataValue *value, SdfSpecType *specType) const;

    /// Fill \p specType (which cannot be nullptr) as if by a call to
    /// GetSpecType(path).  If the resulting specType is not SdfSpecTypeUnknown,
    /// then act as if Has(path, fieldName, value) was called and return its
    /// result.  In other words, the semantics of this function must be
    /// identical to this sequence:
    ///
    /// \code
    /// *specType = GetSpecType(path);
    /// return *specType != SdfSpecTypeUnknown && Has(path, fieldName, value);
    /// \endcode
    SDF_API
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    VtValue *value, SdfSpecType *specType) const;

    /// Return the value for the given \a path and \a fieldName. Returns an
    /// empty value if none is set.
    SDF_API
    virtual VtValue Get(const SdfPath& path, 
                        const TfToken& fieldName) const = 0;

    /// Return the type of the value for \p fieldName on spec \p path.  If no
    /// such field exists, return typeid(void).  Derived classes may optionally
    /// override this for performance.  The base implementation is equivalent
    /// to:
    ///
    /// \code
    /// return Get(path, fieldName).GetTypeid();
    /// \endcode
    SDF_API
    virtual std::type_info const &
    GetTypeid(const SdfPath &path, const TfToken &fieldName) const;

    /// Set the value of the given \a path and \a fieldName.
    ///
    /// It's an error to set a field on a spec that does not exist. Setting a
    /// field to an empty VtValue is the same as calling Erase() on it.
    SDF_API
    virtual void Set(const SdfPath &path, const TfToken &fieldName,
                     const VtValue &value) = 0;

    /// Set the value of the given \a path and \a fieldName.
    ///
    /// It's an error to set a field on a spec that does not exist.
    SDF_API
    virtual void Set(const SdfPath &path, const TfToken &fieldName,
                     const SdfAbstractDataConstValue& value) = 0;

    /// Remove the field at \p path and \p fieldName, if one exists.
    SDF_API
    virtual void Erase(const SdfPath& path, 
                       const TfToken& fieldName) = 0;

    /// Return the names of all the fields that are set at \p path.
    SDF_API
    virtual std::vector<TfToken> List(const SdfPath& path) const = 0;

    /// Return the value for the given \a path and \a fieldName. Returns the
    /// provided \a defaultValue value if none is set.
    template <class T>
    inline T GetAs(const SdfPath& path, const TfToken& fieldName,
                   const T& defaultValue = T()) const;

    /// @}


    /// \name Dict key access API
    /// @{

    // Return true and set \p value (if non null) if the field identified by
    // \p path and \p fieldName is dictionary-valued, and if there is an element
    // at \p keyPath in that dictionary.  Return false otherwise.  If
    // \p keyPath names an entire sub-dictionary, set \p value to that entire
    // sub-dictionary and return true.
    SDF_API
    virtual bool HasDictKey(const SdfPath& path,
                            const TfToken &fieldName,
                            const TfToken &keyPath,
                            SdfAbstractDataValue* value) const;
    SDF_API
    virtual bool HasDictKey(const SdfPath& path,
                            const TfToken &fieldName,
                            const TfToken &keyPath,
                            VtValue *value = NULL) const;

    // Same as HasDictKey but return empty VtValue on failure.
    SDF_API
    virtual VtValue GetDictValueByKey(const SdfPath& path,
                                      const TfToken &fieldName,
                                      const TfToken &keyPath) const;

    // Set the element at \p keyPath in the dictionary-valued field identified
    // by \p path and \p fieldName.  If the field itself is not
    // dictionary-valued, replace the field with a new dictionary and set the
    // element at \p keyPath in it.  If \p value is empty, invoke
    // EraseDictValueByKey instead.
    SDF_API
    virtual void SetDictValueByKey(const SdfPath& path,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath,
                                   const VtValue &value);
    SDF_API
    virtual void SetDictValueByKey(const SdfPath& path,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath,
                                   const SdfAbstractDataConstValue& value);

    // If \p path and \p fieldName identify a dictionary-valued field with an
    // element at \p keyPath, remove that element from the dictionary.  If this
    // leaves the dictionary empty, Erase() the entire field.
    SDF_API
    virtual void EraseDictValueByKey(const SdfPath& path,
                                     const TfToken &fieldName,
                                     const TfToken &keyPath);

    // If \p path, \p fieldName, and \p keyPath identify a (sub) dictionary,
    // return a vector of the keys in that dictionary, otherwise return an empty
    // vector.
    SDF_API
    virtual std::vector<TfToken> ListDictKeys(const SdfPath& path,
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

    SDF_API
    virtual std::set<double>
    ListAllTimeSamples() const = 0;
    
    SDF_API
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfPath& path) const = 0;

    SDF_API
    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, 
                             double* tUpper) const = 0;

    SDF_API
    virtual size_t
    GetNumTimeSamplesForPath(const SdfPath& path) const = 0;

    SDF_API
    virtual bool
    GetBracketingTimeSamplesForPath(const SdfPath& path, 
                                    double time,
                                    double* tLower, double* tUpper) const = 0;

    /// Returns the previous time sample authored just before the querying \p 
    /// time.
    ///
    /// If there is no time sample authored just before \p time, this function
    /// returns false. Otherwise, it returns true and sets \p tPrevious to the
    /// time of the previous sample.
    ///
    /// \note The base class implementation provides an inefficient 
    /// implementation by searching for bracketing time samples twice, if 
    /// \p time happens to land on an authored time sample. Its recommended to 
    /// override this method with a more efficient implementation catering to 
    /// the specific data representation.
    SDF_API
    virtual bool
    GetPreviousTimeSampleForPath(const SdfPath& path, double time, 
                                 double* tPrevious) const;

    SDF_API
    virtual bool
    QueryTimeSample(const SdfPath& path, double time,
                    VtValue *optionalValue = NULL) const = 0;
    SDF_API
    virtual bool
    QueryTimeSample(const SdfPath& path, double time,
                    SdfAbstractDataValue *optionalValue) const = 0;

    SDF_API
    virtual void
    SetTimeSample(const SdfPath& path, double time, 
                  const VtValue & value) = 0;

    SDF_API
    virtual void
    EraseTimeSample(const SdfPath& path, double time) = 0;

    /// @}

protected:
    /// Visits every spec in this SdfAbstractData object with the given 
    /// \p visitor. The order in which specs are visited is undefined. 
    /// The visitor may not modify the SdfAbstractData object it is visiting.
    /// This method should \b not call \c Done() on the visitor.
    /// \sa SdfAbstractDataSpecVisitor
    SDF_API
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const = 0;
};

template <class T>
inline T SdfAbstractData::GetAs(
    const SdfPath& path, 
    const TfToken& field, const T& defaultVal) const
{
    VtValue val = Get(path, field);
    if (val.IsHolding<T>()) {
        return val.UncheckedGet<T>();
    }
    return defaultVal;
}

/// \class SdfAbstractDataValue
///
/// A type-erased container for a field value in an SdfAbstractData.
///
/// \sa SdfAbstractDataTypedValue
class SdfAbstractDataValue
{
public:
    template <class T> 
    bool StoreValue(T &&v) 
    {
        // this can be std::remove_cvref_t in c++20.
        using Type = std::remove_cv_t<std::remove_reference_t<T>>;

        if constexpr (std::is_same_v<Type, VtValue>) {
            return _StoreVtValue(std::forward<T>(v));
        }
        
        isValueBlock = false;
        typeMismatch = false;
        if constexpr (std::is_same_v<Type, SdfValueBlock>) {
            isValueBlock = true;
            return true;
        }
        if (TfSafeTypeCompare(typeid(Type), valueType)) {
            *static_cast<Type*>(value) = std::forward<T>(v);
            return true;
        }
        typeMismatch = true;
        return false;
    }

    void* value;
    const std::type_info& valueType;
    bool isValueBlock;
    bool typeMismatch;

protected:
    SdfAbstractDataValue(void* value_, const std::type_info& valueType_)
        : value(value_)
        , valueType(valueType_)
        , isValueBlock(false)
        , typeMismatch(false)
    { }

private:
    virtual bool _StoreVtValue(const VtValue& value) = 0;
    virtual bool _StoreVtValue(VtValue &&value) = 0;
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
///
template <class T>
class SdfAbstractDataTypedValue : public SdfAbstractDataValue
{
public:
    SdfAbstractDataTypedValue(T* value)
        : SdfAbstractDataValue(value, typeid(T))
    { }

private:
    T const &_Get(const VtValue &v) {
        return v.UncheckedGet<T>();
    }

    T _Get(VtValue &&v) {
        return v.UncheckedRemove<T>();
    }

    template <class Value>
    bool _StoreVtValueImpl(Value &&v) {
        typeMismatch = false;
        isValueBlock = false;
        if (ARCH_LIKELY(std::forward<Value>(v).template IsHolding<T>())) {
            *static_cast<T*>(value) = _Get(std::forward<Value>(v));
            if (std::is_same_v<T, SdfValueBlock>) {
                isValueBlock = true;
            }
            return true;
        }
        
        if (std::forward<Value>(v).template IsHolding<SdfValueBlock>()) {
            isValueBlock = true;
            return true;
        }

        typeMismatch = true;

        return false;
    }
    
    virtual bool
    _StoreVtValue(const VtValue& v) override {
        return _StoreVtValueImpl(v);
    }

    virtual bool
    _StoreVtValue(VtValue &&v) override {
        return _StoreVtValueImpl(v);
    }
};

/// \class SdfAbstractDataConstValue
///
/// A type-erased container for a const field value in an SdfAbstractData.
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
    { 
    }
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
///
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
        return (v.IsHolding<T>() && v.UncheckedGet<T>() == _GetValue());
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
///
/// Base class for objects used to visit specs in an SdfAbstractData object.
///
/// \sa SdfAbstractData::VisitSpecs.
class SdfAbstractDataSpecVisitor
{
public:
    SDF_API
    virtual ~SdfAbstractDataSpecVisitor();

    /// \c SdfAbstractData::VisitSpecs calls this function for every entry it
    /// contains, passing itself as \p data and the entry's \p path.  Return
    /// false to stop iteration early, true to continue.
    SDF_API
    virtual bool VisitSpec(const SdfAbstractData& data, 
                           const SdfPath& path) = 0;

    /// \c SdfAbstractData::VisitSpecs will call this after visitation is
    /// complete, even if some \c VisitSpec() returned \c false.
    SDF_API
    virtual void Done(const SdfAbstractData& data) = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_ABSTRACT_DATA_H
