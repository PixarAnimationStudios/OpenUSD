//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_ENUM_H
#define PXR_BASE_TF_PY_ENUM_H

/// \file tf/pyEnum.h
/// Provide facilities for wrapping enums for script.

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/tf/pySafePython.h"
#include <nanobind/nanobind.h>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_API nanobind::module_ TfPyGetCurrentNanobindModule();

/// \class Tf_PyEnum
///
/// Base class of all python enum classes.
class Tf_PyEnum { };

/// \class Tf_PyEnumRegistry
///
/// This is a private class that manages registered enum objects.
/// \private
class Tf_PyEnumRegistry {

  public:
    typedef Tf_PyEnumRegistry This;
    
  private:
    Tf_PyEnumRegistry();
    virtual ~Tf_PyEnumRegistry();
    friend class TfSingleton<This>;
    
  public:

    TF_API static This &GetInstance() {
        return TfSingleton<This>::GetInstance();
    }
    
    TF_API
    void RegisterValue(TfEnum const &e, nanobind::handle obj);

    TF_API
    PyObject *GetPyObject(TfEnum const &e) {
        return _ConvertEnumToPython(e);
    }

    TF_API
    bool FindEnum(PyObject *obj, TfEnum *out) const;

    template <typename T>
    void RegisterEnumConversions() {
    }

  private:

    TF_API
    PyObject *_ConvertEnumToPython(TfEnum const &e);

    // Since our enum objects live as long as the registry does, we can use the
    // pointer values for a hash.
    struct _ObjectHash {
        size_t operator()(PyObject *o) const {
            return reinterpret_cast<size_t>(o);
        }
    };
    
    TfHashMap<TfEnum, PyObject *, TfHash> _enumsToObjects;
    TfHashMap<PyObject *, TfEnum, _ObjectHash> _objectsToEnums;

};

TF_API_TEMPLATE_CLASS(TfSingleton<Tf_PyEnumRegistry>);

// Private function used for __repr__ of wrapped enum types.
TF_API
std::string Tf_PyEnumRepr(nanobind::handle self);

// Private base class for types which are instantiated and exposed to python
// for each registered enum type.
struct Tf_PyEnumWrapper : public Tf_PyEnum
{
    typedef Tf_PyEnumWrapper This;

    Tf_PyEnumWrapper(std::string const &n, TfEnum const &val) :
        name(n), value(val) {}
    long GetValue() const {
        return value.GetValueAsInt();
    }
    std::string GetName() const{
        return name;
    }
    std::string GetDisplayName() const {
        return TfEnum::GetDisplayName(value);
    }
    std::string GetFullName() const {
        return TfEnum::GetFullName(value);
    }
    friend bool operator ==(Tf_PyEnumWrapper const &self,
                            long other) {
        return self.value.GetValueAsInt() == other;
    }

    friend bool operator ==(Tf_PyEnumWrapper const &lhs,
                            Tf_PyEnumWrapper const &rhs) {
        return lhs.value == rhs.value;
    }

    friend bool operator !=(Tf_PyEnumWrapper const &lhs,
                            Tf_PyEnumWrapper const &rhs) {
        return !(lhs == rhs);
    }

    friend bool operator <(Tf_PyEnumWrapper const &lhs,
                           Tf_PyEnumWrapper const &rhs)
    {
        // If same, not less.
        if (lhs == rhs)
            return false;
        // If types don't match, string compare names.
        if (!lhs.value.IsA(rhs.value.GetType()))
            return TfEnum::GetFullName(lhs.value) <
                TfEnum::GetFullName(rhs.value);
        // If types do match, numerically compare values.
        return lhs.GetValue() < rhs.GetValue();
    }

    friend bool operator >(Tf_PyEnumWrapper const& lhs,
                           Tf_PyEnumWrapper const& rhs)
    {
        return rhs < lhs;
    }

    friend bool operator <=(Tf_PyEnumWrapper const& lhs,
                            Tf_PyEnumWrapper const& rhs)
    {
        return !(lhs > rhs);
    }

    friend bool operator >=(Tf_PyEnumWrapper const& lhs,
                            Tf_PyEnumWrapper const& rhs)
    {
        return !(lhs < rhs);
    }

    //
    // XXX Bitwise operators for Enums are a temporary measure to support the
    // use of Enums as Bitmasks in libSd.  It should be noted that Enums are
    // NOT closed under these operators. The proper place for such operators
    // is in a yet-nonexistent Bitmask type.  
    //

    friend TfEnum operator |(Tf_PyEnumWrapper const &lhs,
                        Tf_PyEnumWrapper const &rhs) {
        if (lhs.value.IsA(rhs.value.GetType())) {
            return TfEnum(lhs.value.GetType(),
                          lhs.value.GetValueAsInt() |
                          rhs.value.GetValueAsInt());
        }
        throw nanobind::type_error("Enum type mismatch");
        return TfEnum();
    }
    friend TfEnum operator |(Tf_PyEnumWrapper const &lhs, long rhs) {
        return TfEnum(lhs.value.GetType(), lhs.value.GetValueAsInt() | rhs);
    }
    friend TfEnum operator |(long lhs, Tf_PyEnumWrapper const &rhs) {
        return TfEnum(rhs.value.GetType(), lhs | rhs.value.GetValueAsInt());
    }
    
    friend TfEnum operator &(Tf_PyEnumWrapper const &lhs,
                             Tf_PyEnumWrapper const &rhs) {
        if (lhs.value.IsA(rhs.value.GetType())) {
            return TfEnum(lhs.value.GetType(),
                          lhs.value.GetValueAsInt() &
                          rhs.value.GetValueAsInt());
        }
        throw nanobind::type_error("Enum type mismatch");
        return TfEnum();
    }
    friend TfEnum operator &(Tf_PyEnumWrapper const &lhs, long rhs) {
        return TfEnum(lhs.value.GetType(), lhs.value.GetValueAsInt() & rhs);
    }
    friend TfEnum operator &(long lhs, Tf_PyEnumWrapper const &rhs) {
        return TfEnum(rhs.value.GetType(), lhs & rhs.value.GetValueAsInt());
    }
    
    friend TfEnum operator ^(Tf_PyEnumWrapper const &lhs,
                             Tf_PyEnumWrapper const &rhs) {
        if (lhs.value.IsA(rhs.value.GetType())) {
            return TfEnum(lhs.value.GetType(),
                          lhs.value.GetValueAsInt() ^
                          rhs.value.GetValueAsInt());
        }
        throw nanobind::type_error("Enum type mismatch");
        return TfEnum();
    }
    friend TfEnum operator ^(Tf_PyEnumWrapper const &lhs, long rhs) {
        return TfEnum(lhs.value.GetType(), lhs.value.GetValueAsInt() ^ rhs);
    }
    friend TfEnum operator ^(long lhs, Tf_PyEnumWrapper const &rhs) {
        return TfEnum(rhs.value.GetType(), lhs ^ rhs.value.GetValueAsInt());
    }
    
    friend TfEnum operator ~(Tf_PyEnumWrapper const &rhs) {
        return TfEnum(rhs.value.GetType(), ~rhs.value.GetValueAsInt());
    }
    std::string name;
    TfEnum value;
};

// Private template class which is instantiated and exposed to python for each
// registered enum type.
template <typename T>
struct Tf_TypedPyEnumWrapper : Tf_PyEnumWrapper
{
    Tf_TypedPyEnumWrapper(std::string const &n, TfEnum const &val) :
        Tf_PyEnumWrapper(n, val) {}

    static nanobind::object GetValueFromName(const std::string& name) {
        bool found = false;
        const TfEnum value = TfEnum::GetValueFromName<T>(name, &found);
        if (!found) {
            return nanobind::none();
        }
        return nanobind::steal(
            Tf_PyEnumRegistry::GetInstance().GetPyObject(value));
    }
};

// Sanitizes the given \p name for use as a Python identifier. This includes
// replacing spaces with '_' and appending '_' to names matching Python 
// keywords.
// 
// If \p stripPackageName is true and \p name begins with the package name,
// it will be stripped off.
TF_API
std::string Tf_PyCleanEnumName(std::string name,
                               bool stripPackageName = false);

// Adds attribute of given name with given value to given scope.
// Issues a coding error if attribute by that name already existed.
TF_API
void Tf_PyEnumAddAttribute(nanobind::handle s,
                           const std::string &name,
                           nanobind::handle value);

// Detect scoped enums by using that the C++ standard does not allow them to
// be converted to int implicitly.
template <typename T, bool IsScopedEnum = !std::is_convertible<T, int>::value>
struct TfPyWrapEnum {

private:
    typedef nanobind::class_<Tf_TypedPyEnumWrapper<T>, Tf_PyEnumWrapper>
        _EnumPyClassType;

public:

    /// Construct an enum wrapper object.
    /// If \a name is provided, it is used as the name of the enum.  Otherwise
    /// the type name of \a T is used, with a leading MFB package name
    /// stripped.
    explicit TfPyWrapEnum(std::string const &name = std::string())
        : TfPyWrapEnum(TfPyGetCurrentNanobindModule(), name)
    {
    }

    TfPyWrapEnum(nanobind::handle scope,
                 std::string const &name = std::string())
    {
        const bool explicitName = !name.empty();

        // First, take either the given name, or the demangled type name.
        std::string enumName = explicitName ? name :
            TfStringReplace(ArchGetDemangled(typeid(T)), "::", ".");

        // If the name is dotted, take everything before the dot as the base
        // name.  This is used in repr.
        std::string baseName = TfStringGetBeforeSuffix(enumName);
        if (baseName == enumName)
            baseName = std::string();

        // If the name is dotted, take the last element as the enum name.
        if (!TfStringGetSuffix(enumName).empty())
            enumName = TfStringGetSuffix(enumName);

        // If the name was not explicitly given, then clean it up by removing
        // the package name prefix if it exists.
        if (!explicitName) {
            if (!baseName.empty()) {
                baseName = Tf_PyCleanEnumName(
                    baseName, /* stripPackageName = */ true);
            }
            else {
                enumName = Tf_PyCleanEnumName(
                    enumName, /* stripPackageName = */ true);
            }
        }
        
        if (IsScopedEnum) {
            // Make the enumName appear in python representation
            // for scoped enums.
            if (!baseName.empty()) {
                baseName += ".";
            }
            baseName += enumName;
        }

        // Make a python type for T.
        _EnumPyClassType enumClass(scope, enumName.c_str());
        enumClass.def_static("GetValueFromName",
                             &Tf_TypedPyEnumWrapper<T>::GetValueFromName,
                             nanobind::arg("name"));
        enumClass.attr("_baseName") = baseName;

        // Register conversions for it.
        Tf_PyEnumRegistry::GetInstance().RegisterEnumConversions<T>();

        // Export values.
        const bool stripPackageName = baseName.empty();
        _ExportValues(stripPackageName, scope, enumClass);

        // Register with Tf so that python clients of a TfType
        // that represents an enum are able to get to the equivalent 
        // python class with .pythonclass
        const TfType &type = TfType::Find<T>();
        if (!type.IsUnknown()) {
            type.DefinePythonClass(enumClass);
        }
    }
    
  private:

    /// Export all values in this enum to the enclosing scope.
    /// If no explicit names have been registered, this will export the TfEnum
    /// registered names and values (if any).
    void _ExportValues(bool stripPackageName,
                       nanobind::handle enclosingScope,
                       _EnumPyClassType &enumClass) {
        nanobind::list valueList;

        for (const std::string& name : TfEnum::GetAllNames<T>()) {
            bool success = false;
            TfEnum enumValue = TfEnum::GetValueFromName<T>(name, &success);
            if (!success) {
                continue;
            }

            const std::string cleanedName = 
                Tf_PyCleanEnumName(name, stripPackageName);

            // convert value to python.
            Tf_TypedPyEnumWrapper<T> wrappedValue(cleanedName, enumValue);
            nanobind::object pyValue = nanobind::cast(wrappedValue);

            // register it as the python object for this value.
            Tf_PyEnumRegistry::GetInstance().RegisterValue(enumValue, pyValue);

            // Take all the values and export them into the current scope.
            std::string valueName = wrappedValue.GetName();
            if (IsScopedEnum) {
                // If scoped enum, enum values appear on the enumClass ...
                Tf_PyEnumAddAttribute(enumClass, valueName, pyValue);
            } else {
                // ... otherwise, enum values appear on the enclosing scope.
                Tf_PyEnumAddAttribute(enclosingScope, valueName, pyValue);
            }

            valueList.append(pyValue);
        }

        // Add a tuple of all the values to the enum class.
        enumClass.attr("allValues") = nanobind::tuple(valueList);
    }

};

PXR_NAMESPACE_CLOSE_SCOPE

#include "pxr/base/tf/pyUtils.h"

#endif // PXR_BASE_TF_PY_ENUM_H
