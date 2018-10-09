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
#ifndef TF_ENUM_H
#define TF_ENUM_H

/// \file tf/enum.h
/// \ingroup group_tf_RuntimeTyping

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/preprocessorUtils.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/api.h"

#include <boost/operators.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/utility/enable_if.hpp>

#include <iosfwd>
#include <string>
#include <typeinfo>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfEnum
/// \ingroup group_tf_RuntimeTyping
///
/// An enum class that records both enum type and enum value.
///
/// \section cppcode_runtimeTyping Run-Time Typing 
///
/// A \c TfEnum can hold an enum variable of any enum type, while still
/// being able to distinguish between various enum types.
/// Here is an example:
///
/// \code
/// enum Monsters { SULLEY = 0, MIKE, ROZ };
/// enum Fish { NEMO = 0, FATHER, DORY };
///
/// TfEnum t1 = MIKE,
///        t2 = NEMO;
///
/// t1 == MIKE;        // yields true
/// t2 == NEMO;        // yields true
/// t1 == t2;          // yields false
/// t1 == SULLEY;      // yields false
///
/// t1.IsA<Monsters>();        // yields true
/// t1.IsA<Fish>();    // yields false
/// \endcode
///
/// Even though \c NEMO and \c SULLEY both are represented with integral
/// value zero, \c t1 and \c t2 compare false.  A \c TfEnum can be passed
/// by value, assigned, etc. just like a regular \c Enum variable.
/// A \c TfEnum can also hold a plain integer, which will compare false against
/// any other enum variable.
///
/// \section cppcode_enumvals Associating Names with Enumerated Values 
///
/// The \c TfEnum class can also be used to represent enumerated values
/// as strings. This can be useful for storing enum values in files for
/// later retrieval.
///
/// Use the \c TF_ADD_ENUM_NAME() macro to set up and enable strings
/// for the values of an enum. Once this is done, several static 
/// \c TfEnum methods may be used to look up names corresponding to enum
/// values and vice-versa.
///
/// For example, see \c TfRegistryManager to understand the use of
/// the \c TF_REGISTRY_FUNCTION() macro below:
///
/// \section cppcode_enumRegMacro Enum Registration Macro
///
/// \code
/// // header file
/// // Declare an enumerated type with some values
/// enum Season {
///     SPRING,
///     SUMMER = 3, // It's ok to have initializers
///     AUTUMN,
///     WINTER
/// };
///
/// // source file
/// #include "pxr/base/tf/registryManager.h"
/// TF_REGISTRY_FUNCTION(TfEnum) {
///     // Register the names for the values:
///     TF_ADD_ENUM_NAME(SPRING);
///     TF_ADD_ENUM_NAME(SUMMER);
///     TF_ADD_ENUM_NAME(AUTUMN);
///     TF_ADD_ENUM_NAME(WINTER);
/// }
///
/// // another source file:
///
/// // Look up the name for a value:
/// string name1 = TfEnum::GetName(SUMMER);     // Returns "SUMMER"
/// string name2 = TfEnum::GetFullName(SUMMER); // Returns "Season::SUMMER"
///
/// // Look up the value for a name:
/// bool found;
/// Season s1 = TfEnum::GetValueFromName<Season>("AUTUMN", &found);
/// // Returns 4, sets found to true
/// Season s2 = TfEnum::GetValueFromName<Season>("MONDAY", &found);
/// // Returns -1, sets found to false
///
/// // Look up a fully-qualified name. Since this is not a templated
/// // function, it has to return a generic value type, so we use
/// // TfEnum.
/// TfEnum s3 = TfEnum::GetValueFromFullName("Season::WINTER", &found);
/// // Returns 5, sets found to \c true
/// \endcode
///
class TfEnum : boost::totally_ordered<TfEnum>
{
public:
    /// Default constructor assigns integer value zero.
    TfEnum()
        : _typeInfo(&typeid(int)), _value(0)
    {
    }

    /// Initializes value to enum variable \c value of enum type \c T.
    template <class T>
    TfEnum(T value,
           typename boost::enable_if<boost::is_enum<T> >::type * = 0)
        : _typeInfo(&typeid(T)), _value(int(value))
    {
    }

    /// Initializes value to integral value \p value with enum type \c ti.
    ///
    /// \warning This is only for use in extreme circumstances; there is no
    /// way for an implementation to guarantee that \p ti is really an enum
    /// type, and/or that \p value is a valid value for that enum type.
    TfEnum(const std::type_info& ti, int value)
        : _typeInfo(&ti), _value(value)
    {
    }

    /// True if \c *this and \c t have both the same type and value.
    bool operator==(const TfEnum& t) const {
        return t._value == _value &&
            TfSafeTypeCompare(*t._typeInfo, *_typeInfo);
    }

    /// Less than comparison. Enum values belonging to the same type are
    /// ordered according to their numeric value.  Enum values belonging to
    /// different types are ordered in a consistent but arbitrary way which
    /// may vary between program runs.
    bool operator<(const TfEnum& t) const {
        return _typeInfo->before(*t._typeInfo) ||
            (!t._typeInfo->before(*_typeInfo) && _value < t._value);
    }

    /// True if \c *this has been assigned with \c value.
    template <class T>
    typename boost::enable_if<boost::is_enum<T>, bool>::type
    operator==(T value) const {
        return int(value) == _value && IsA<T>();
    }

    /// False if \c *this has been assigned with \c value.
    template <class T>
    typename boost::enable_if<boost::is_enum<T>, bool>::type
    operator!=(T value) const {
        return int(value) != _value || !IsA<T>();
    }

    /// Compare a literal enum value \a val of enum type \a T with TfEnum \a e.
    template <class T>
    friend typename boost::enable_if<boost::is_enum<T>, bool>::type
    operator==(T val, TfEnum const &e) {
        return e == val;
    }

    /// Compare a literal enum value \a val of enum type \a T with TfEnum \a e.
    template <class T>
    friend typename boost::enable_if<boost::is_enum<T>, bool>::type
    operator!=(T val, TfEnum const &e) {
        return e != val;
    }

    /// True if \c *this has been assigned any enumerated value of type \c T.
    template <class T>
    bool IsA() const {
        return TfSafeTypeCompare(*_typeInfo, typeid(T));
    }
    
    /// True if \c *this has been assigned any enumerated value of type
    /// \c T with \c typeid(T)==t.
    bool IsA(const std::type_info& t) const {
        return TfSafeTypeCompare(*_typeInfo, t);
    }

    /// Returns the type of the enum value, as an \c std::type_info.
    const std::type_info& GetType() const {
        return *_typeInfo;
    }

    /// Returns the integral value of the enum value.
    const int& GetValueAsInt() const {
        return _value;
    }

    /// Returns the enum value for the enum type \c T.
    ///
    /// \warning This function can cause your program to abort if not used
    /// properly.
    ///
    /// If it is possible that the enum value is not of type \c T, first use
    /// \c IsA() to test whether the enum value is of type \c T before calling
    /// \c GetValue<T>().
    ///
    /// Note that if \c IsA<T>() succeeds, then \c GetValue<T>() will also
    /// succeed.
    template <typename T>
    T GetValue() const {
        if (!IsA<T>())
            _FatalGetValueError(typeid(T));

        return T(_value);
    }

    /// Conversion operator for enum and integral types only.
    template <typename T,
             typename = typename std::enable_if<
                 std::is_integral<T>::value ||
                 std::is_enum<T>::value>::type
             >
    operator T() const
    {
        return T(_value);
    }

    /// \name Retrieving Corresponding Names and Enumerated Values
    ///
    /// The methods in this group can be used to retrieve corresponding names
    /// and values. The correspondences are set up with the
    /// \c TF_ADD_ENUM_NAME() macro.
    ///
    ///@{

    /// Returns the name associated with an enumerated value.
    ///
    /// If there is no such name registered, an empty string is returned.
    TF_API static std::string  GetName(TfEnum val);

    /// Returns the fully-qualified name for an enumerated value.
    ///
    /// This returns a fully-qualified enumerated value name (e.g.,
    /// \c "Season::WINTER") associated with the given value. If there is no
    /// such name registered, an empty string is returned.
    TF_API static std::string  GetFullName(TfEnum val);

    /// Returns the display name for an enumerated value.
    ///
    /// This returns a user interface-suitable string for the given enumerated
    /// value.
    TF_API static std::string  GetDisplayName(TfEnum val);

    /// Returns a vector of all the names associated with an enum type.
    ///
    /// This returns a vector of all the names associated with the enum that
    /// contains the type \p val.  The names are not fully qualified.  For
    /// example, \c TfEnum::GetAllNames(WINTER) would return a vector
    /// containing "SPRING", "SUMMER", "AUTUMN", and "WINTER".
    ///
    /// If there are no such names registered, an empty vector is returned.
    static std::vector<std::string> GetAllNames(TfEnum val) {
        return GetAllNames(val.GetType());
    }
    
    /// \overload
    TF_API static std::vector<std::string> GetAllNames(const std::type_info &ti);

    /// Returns a vector of all the names associated with an enum type.
    ///
    /// This returns a vector of all the names associated with the enum 
    /// type \c T.  The names are not fully qualified.  For
    /// example, \c TfEnum::GetAllNames<Season>() would return a vector
    /// containing "SPRING", "SUMMER", "AUTUMN", and "WINTER".
    ///
    /// If there are no such names registered, an empty vector is returned.
    template <class T>
    static std::vector<std::string> GetAllNames() {
        return GetAllNames(typeid(T));
    }

    /// Returns the typeid for a given enum type name.
    ///
    /// This returns a pointer to the type_info associated with the enum that
    /// has the type name \c typeName.  If no such enum is registered, returns
    /// NULL.
    TF_API
    static const std::type_info *GetTypeFromName(const std::string& typeName);

    /// Returns the enumerated value for a name.
    ///
    /// If there is no such name registered, this returns -1. Since -1 can
    /// sometimes be a valid value, the \p foundIt flag pointer, if not \c
    /// NULL, is set to \c true if the name was found and \c false otherwise.
    template <class T>
    static T GetValueFromName(const std::string &name, bool *foundIt = NULL) {
        TfEnum e = GetValueFromName(typeid(T), name, foundIt);
        return T(e.GetValueAsInt());
    }

    /// Returns the enumerated value for a name.
    ///
    /// This is a template-independent version of \c GetValueFromName().
    TF_API
    static TfEnum GetValueFromName(const std::type_info& ti,
                                   const std::string &name,
                                   bool *foundIt = NULL);

    /// Returns the enumerated value for a fully-qualified name.
    ///
    /// This takes a fully-qualified enumerated value name (e.g.,
    /// \c "Season::WINTER") and returns the associated value. If there is
    /// no such name, this returns -1. Since -1 can sometimes be a
    /// valid value, the \p foundIt flag pointer, if not \c NULL, is
    /// set to \c true if the name was found and \c false
    /// otherwise. Also, since this is not a templated function, it has
    /// to return a generic value type, so we use \c TfEnum.
    TF_API
    static TfEnum GetValueFromFullName(const std::string &fullname,
                                       bool *foundIt = NULL);

    /// Returns true if \p typeName is a known enum type.
    ///
    /// If any enum whose demangled type name is \p typeName has been
    /// added via \c TF_ADD_ENUM_NAME(), this function returns true.
    TF_API
    static bool IsKnownEnumType(const std::string& typeName);

    ///@}

    /// Associates a name with an enumerated value.
    ///
    /// \warning This method is called by the \c TF_ADD_ENUM_NAME() macro, and
    /// should NOT be called directly. Instead, call AddName(), which does
    /// exactly the same thing.
    TF_API
    static void _AddName(TfEnum val, const std::string &valName,
                         const std::string &displayName="");

    /// Associates a name with an enumerated value.
    /// \see _AddName().
    static void AddName(TfEnum val, const std::string &valName,
                        const std::string &displayName="")
    {
        _AddName(val, valName, displayName);
    }
     
    template <typename T>
    static TfEnum IntegralEnum(T value) {
        TfEnum e;
        e._typeInfo = &typeid(T);
        e._value = int(value);
        return e;
    }

private:
    // Internal constructor for int values.
    explicit TfEnum(int value)
        : _typeInfo(&typeid(int)), _value(value)
    {
    }

    // Internal constructor for size_t values.
    explicit TfEnum(size_t value)
        : _typeInfo(&typeid(size_t)), _value(static_cast<int>(value))
    {
    }

    TF_API
    void _FatalGetValueError(std::type_info const& typeInfo) const;

    const std::type_info* _typeInfo;
    int _value;
};

/// Output a TfEnum value.
/// \ingroup group_tf_DebuggingOutput
TF_API std::ostream& operator<<(std::ostream& out, const TfEnum & e);

/// Macro used to associate a name with an enumerated value.
///
/// \c TF_ADD_ENUM_NAME() registers a name for an enumerated value so that the
/// association can be accessed in calls to \c TfEnum::GetValueFromName(),
/// \c TfEnum::GetValueFromFullName(), \c TfEnum::GetName(), and
/// \c TfEnum::GetFullName(). It's first argument, \p VAL, is the symbolic
/// name of the enumerated value or a \c TfEnum instance constructed from it.
/// The name defined for the value is created by putting double quotes around
/// the \p VAL argument.
///
/// An optional second argument, \p DISPLAY, is a name to be used for display
/// purposes (i.e. in a user interface). The display name can contain
/// characters like spaces and punctuation, and does not need to be a unique
/// string. If this argument is not specified, the display name will be
/// derived from \p VAL.
///
/// Only the names for which \c TF_ADD_ENUM_NAME() is called will be
/// accessible with the name/value methods; you can hide values from this
/// mechanism by not adding them.
///
/// Please note that the best way to call \c TF_ADD_ENUM_NAME() is using the
/// \c TfRegistryManager macro \c TF_REGISTRY_FUNCTION().
///
/// \ingroup group_tf_RuntimeTyping
/// \hideinitializer
#define TF_ADD_ENUM_NAME(VAL, ...)                                      \
    TfEnum::_AddName(VAL,                                               \
                     BOOST_PP_STRINGIZE(VAL)                            \
                     BOOST_PP_COMMA_IF(TF_NUM_ARGS(__VA_ARGS__))        \
                     __VA_ARGS__)

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_ENUM_H
