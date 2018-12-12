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
#ifndef TF_TYPE_H
#define TF_TYPE_H

#include "pxr/pxr.h"

#include "pxr/base/tf/api.h"
#include "pxr/base/tf/cxxCast.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/traits.h"
#include "pxr/base/tf/typeFunctions.h"

#include <boost/mpl/vector.hpp>
#include <boost/operators.hpp>

#include <iosfwd>
#include <memory>
#include <set>
#include <type_traits>
#include <typeinfo>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

#ifdef PXR_PYTHON_SUPPORT_ENABLED
class TfPyObjWrapper;
#endif // PXR_PYTHON_SUPPORT_ENABLED

class TfType;

/// \class TfType
///
/// TfType represents a dynamic runtime type.
///
/// TfTypes are created and discovered at runtime, rather than compile
/// time.
///
/// Features:
///
/// - unique typename
/// - safe across DSO boundaries
/// - can represent C++ types, pure Python types, or Python subclasses of
///   wrapped C++ types
/// - lightweight value semantics -- you can copy and default construct
///   TfType, unlike \c std::type_info.
/// - totally ordered -- can use as a \c std::map key
///
class TfType : boost::totally_ordered<TfType>
{
    struct _TypeInfo;

public:
    /// Callback invoked when a declared type needs to be defined.
    using DefinitionCallback = void (*)(TfType);

    /// Base class of all factory types.
    class FactoryBase {
    public:
        TF_API virtual ~FactoryBase();
    };

public:
    
    enum LegacyFlags {
        ABSTRACT =       0x01,   ///< Abstract (unmanufacturable and unclonable)
        CONCRETE =       0x02,   ///< Not abstract
        MANUFACTURABLE = 0x08,   ///< Manufacturable type (implies concrete)
    };

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    // This is a non-templated base class for the templated
    // polymorphic-to-Python infrastructure.
    struct PyPolymorphicBase
    {
    protected:
        TF_API virtual ~PyPolymorphicBase();
    };
#endif // PXR_PYTHON_SUPPORT_ENABLED

public:
    /// A type-list of C++ base types.
    /// \see TfType::Define()
    template <class ... Args>
    struct Bases : boost::mpl::vector<Args ...> {};

public:
    /// Construct an TfType representing an unknown type.
    ///
    /// To actually register a new type with the TfType system, see
    /// TfType::Declare().
    ///
    /// Note that this always holds true:
    /// \code
    ///     TfType().IsUnknown() == true
    /// \endcode
    ///
    TF_API
    TfType();

    /// Return an empty TfType, representing the unknown type.
    ///
    /// This is equivalento the default constructor, TfType().  This form exists
    /// as a clearer way to express intent in code explicitly dealing with
    /// unknown types.
    ///
    /// \see IsUnknown()
    ///
    TF_API
    static TfType const& GetUnknownType();

    /// Equality operator.
    ///
    /// \note All unknown types (see IsUnknown()) are considered equal.
    /// This is so all unknown types will only occupy one key when used in
    /// an associative map.
    inline bool operator ==(const TfType& t) const { return _info == t._info; }

    /// Comparison operator.
    inline bool operator <(const TfType& t) const { return _info < t._info; }

    
    /// \name Finding types
    /// @{

    /// Retrieve the \c TfType corresponding to type \c T.
    ///
    /// The type \c T must have been defined in the type system or the
    /// \c TfType corresponding to an unknown type is returned.
    ///
    /// \see IsUnknown()
    ///
    template <typename T>
    static TfType const& Find() {
        return Find(typeid(T));
    }

    /// Retrieve the \c TfType corresponding to \c obj.
    ///
    /// The \c TfType corresponding to the actual object represented
    /// by \c obj is returned; this may not be the object returned by
    /// \c TfType::Find<T>() if \c T is a polymorphic type.
    ///
    /// This works for Python subclasses of the C++ type \c T as well,
    /// as long as \c T has been wrapped using TfPyPolymorphic.
    ///
    /// Of course, the object's type must have been defined in the type
    /// system or the \c TfType corresponding to an unknown type is returned.
    ///
    /// \see IsUnknown()
    ///
    template <typename T>
    static TfType const& Find(const T &obj) {
        typedef typename TfTraits::Type<T>::UnderlyingType Type;
        // If T is polymorphic to python, we may have to bridge into python.  We
        // could also optimize for Ts that are not polymorphic at all and avoid
        // doing rtti typeid lookups, but we trust the compiler to do this for
        // us.
        if (Type const *rawPtr = TfTypeFunctions<T>::GetRawPtr(obj))
            return _FindImpl(rawPtr);
        return GetUnknownType();
    }

    /// Retrieve the \c TfType corresponding to an obj with the
    /// given \c type_info.
    ///
    static TfType const& Find(const std::type_info &t) {
        return _FindByTypeid(t);
    }
    
    /// Retrieve the \c TfType corresponding to an obj with the
    /// given \c type_info.
    ///
    static TfType const& FindByTypeid(const std::type_info &t) {
        return _FindByTypeid(t);
    }

    /// Retrieve the \c TfType corresponding to the given \c name.
    ///
    /// Every type defined in the TfType system has a unique, implementation
    /// independent name.  In addition, aliases can be added to identify
    /// a type underneath a specific base type; see TfType::AddAlias().
    /// The given name will first be tried as an alias under the root type,
    /// and subsequently as a typename.
    ///
    /// This method is equivalent to:
    /// \code
    ///    TfType::GetRoot().FindDerivedByName(name)
    /// \endcode
    ///
    /// For any object \c obj,
    /// \code
    ///    Find(obj) == FindByName( Find(obj).GetTypeName() )
    /// \endcode
    ///
    TF_API
    static TfType const& FindByName(const std::string &name);

    /// Retrieve the \c TfType that derives from this type and has the
    /// given alias or typename.
    ///
    /// \see AddAlias
    ///
    TF_API
    TfType const& FindDerivedByName(const std::string &name) const;

    /// Retrieve the \c TfType that derives from BASE and has the
    /// given alias or typename.
    ///
    /// This is a convenience method, and is equivalent to:
    /// \code
    ///    TfType::Find<BASE>().FindDerivedByName(name)
    /// \endcode
    ///
    template <typename BASE>
    static TfType const& FindDerivedByName(const std::string &name)
    {
        return TfType::Find<BASE>().FindDerivedByName(name);
    }

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    /// Retrieve the \c TfType corresponding to an obj with the
    /// given Python class \c classObj.
    ///
    TF_API
    static TfType const& FindByPythonClass(const TfPyObjWrapper & classObj);
#endif // PXR_PYTHON_SUPPORT_ENABLED

    /// @}


    /// \name Type queries
    /// @{

    /// Return the root type of the type hierarchy.
    ///
    /// All known types derive (directly or indirectly) from the root.
    /// If a type is specified with no bases, it is implicitly
    /// considered to derive from the root type.
    ///
    TF_API
    static TfType const& GetRoot();

    /// Return the machine-independent name for this type.
    /// This name is specified when the TfType is declared.
    /// \see Declare()
    ///
    TF_API
    const std::string &GetTypeName() const;

    /// Return a C++ RTTI type_info for this type.
    ///
    /// If this type is unknown or has not yet had a C++ type defined,
    /// \c typeid(void) will be returned.
    ///
    /// \see Define()
    ///
    TF_API
    const std::type_info &GetTypeid() const;

    /// Return the canonical typeName used for a given std::type_info.
    ///
    /// Exactly how the canonical name is generated is left undefined,
    /// but in practice it is likely to be the demangled RTTI name
    /// of the type_info, stripped of namespaces.  The real answer
    /// is implemented by this method.
    ///
    TF_API
    static std::string GetCanonicalTypeName(const std::type_info &);

    /// Returns a vector of the aliases registered for the derivedType
    /// under this, the base type.
    /// \see AddAlias()
    ///
    TF_API
    std::vector<std::string> GetAliases(TfType derivedType) const;

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    /// Return the Python class object for this type.
    ///
    /// If this type is unknown or has not yet had a Python class
    /// defined, this will return \c None, as an empty
    /// \c TfPyObjWrapper
    ///
    /// \see DefinePythonClass()
    ///
    TF_API
    TfPyObjWrapper GetPythonClass() const;
#endif // PXR_PYTHON_SUPPORT_ENABLED

    /// Return a vector of types from which this type was derived.
    ///
    TF_API
    std::vector<TfType> GetBaseTypes() const;

    /// Return a vector of types derived directly from this type.
    ///
    TF_API
    std::vector<TfType> GetDirectlyDerivedTypes() const;

    /// Return the canonical type for this type.
    TF_API
    TfType const& GetCanonicalType() const;
    
    /// Return the set of all types derived (directly or indirectly)
    /// from this type.
    ///
    TF_API
    void GetAllDerivedTypes( std::set<TfType> *result ) const;

    /// Build a vector of all ancestor types inherited by this type.
    /// The starting type is itself included, as the first element of
    /// the results vector.
    ///
    /// Types are given in "C3" resolution order, as used for new-style
    /// classes starting in Python 2.3.  This algorithm is more complicated
    /// than a simple depth-first traversal of base classes, in order to
    /// prevent some subtle errors with multiple-inheritance.  See the
    /// references below for more background.
    ///
    /// \note This can be expensive; consider caching the results.  TfType
    ///   does not cache this itself since it is not needed internally.
    ///
    /// \see Guido van Rossum.
    ///   "Unifying types and classes in Python 2.2: Method resolution order."
    ///   http://www.python.org/download/releases/2.2.2/descrintro/#mro
    ///
    /// \see Barrett, Cassels, Haahr, Moon, Playford, Withington.
    ///   "A Monotonic Superclass Linearization for Dylan."  OOPSLA 96.
    ///   http://www.webcom.com/haahr/dylan/linearization-oopsla96.html
    ///
    TF_API
    void GetAllAncestorTypes(std::vector<TfType> *result) const;

    /// Return true if this type is the same as or derived from \p queryType.
    /// If \c queryType is unknown, this always returns \c false.
    ///
    TF_API
    bool IsA(TfType queryType) const;

    /// Return true if this type is the same as or derived from T.
    /// This is equivalent to:
    /// \code
    ///     IsA(Find<T>())
    /// \endcode
    ///
    template <typename T>
    bool IsA() const { return IsA(Find<T>()); }

    /// Return true if this is the unknown type, representing a type
    /// unknown to the TfType system.
    ///
    /// The unknown type does not derive from the root type, or any
    /// other type.
    ///
    bool IsUnknown() const { return *this == TfType(); }

    typedef TfType::_TypeInfo * (TfType::*UnspecifiedBoolType);

    /// Convert to bool -- return true if this type is not unknown, false
    /// otherwise.
    operator UnspecifiedBoolType() const {
        return IsUnknown() ? NULL : &TfType::_info;
    }

    /// Boolean not operator -- return true if this type is unknown, false
    /// otherwise.
    bool operator !() const { return !bool(*this); }

    /// Return true if this is the root type.
    ///
    bool IsRoot() const { return *this == GetRoot(); }

    /// Return true if this is an enum type.
    ///
    TF_API
    bool IsEnumType() const;

    /// Return true if this is a plain old data type, as defined by C++.
    ///
    TF_API
    bool IsPlainOldDataType() const;

    /// Return the size required to hold an instance of this type on the stack
    /// (does not include any heap allocated memory the instance uses).
    ///
    /// This is what the C++ sizeof operator returns for the type, so this
    /// value is not very useful for Python types (it will always be
    /// sizeof(boost::python::object)).
    ///
    TF_API
    size_t GetSizeof() const;

    /// @}


    /// \name Registering new types
    /// @{

    /// Declare a TfType with the given \c typeName, but no base type
    /// information.  This just establishes the minimal stub for the
    /// type to exist, prior to it being fleshed out with more
    /// declarations (specifying base types) or a definition.
    ///
    TF_API
    static TfType const&
    Declare( const std::string & typeName );

    /// Declare a TfType with the given \c typeName and \c bases.
    /// If the bases vector is empty, the type will be marked as
    /// deriving from the root TfType (see TfType::GetRootType()).
    /// The \c definitionCallback, if given, will be invoked later to
    /// define the type when needed.
    ///
    /// It is ok to redeclare a type that has already been declared.
    /// The given bases will supplement any existing bases.  An
    /// example use of this is the Plugin system, where only a single
    /// base may be known in the plugin metadata, but when the code
    /// is loaded later, a full set of bases is specified.
    ///
    /// It is an error to redeclare a type's definitionCallback.
    ///
    TF_API
    static TfType const&
    Declare( const std::string & typeName,
             const std::vector<TfType> & bases,
             DefinitionCallback definitionCallback=nullptr );

    /// Define a TfType with the given C++ type T and C++ base types
    /// B.  Each of the base types will be declared (but not defined)
    /// as TfTypes if they have not already been.
    ///
    /// The typeName of the created TfType will be the canonical
    /// demangled RTTI type name, as defined by GetCanonicalTypeName().
    ///
    /// It is an error to attempt to define a type that has already
    /// been defined.
    ///
    template <typename T, typename B>
    static TfType const& Define();

    /// Define a TfType with the given C++ type T and no bases.
    /// See the other Define() template for more details.
    ///
    /// \note C++ does not allow default template arguments for function
    /// templates, so we provide this separate definition for the case of
    /// no bases.
    ///
    template <typename T>
    static TfType const& Define();

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    /// Define the Python class object corresponding to this TfType.
    /// \see TfTypePythonClass
    TF_API
    void DefinePythonClass(const TfPyObjWrapper &classObj) const;
#endif // PXR_PYTHON_SUPPORT_ENABLED

    /// Add an alias for DERIVED beneath BASE.
    ///
    /// This is a convenience method, that declares both DERIVED and BASE
    /// as TfTypes before adding the alias.
    ///
    template <typename Base, typename Derived>
    static void AddAlias(const std::string &name) {
        TfType b = Declare(GetCanonicalTypeName(typeid(Base)));
        TfType d = Declare(GetCanonicalTypeName(typeid(Derived)));
        d.AddAlias(b, name);
    }

    /// Add an alias name for this type under the given base type.
    ///
    /// Aliases are similar to typedefs in C++: they provide an
    /// alternate name for a type.  The alias is defined with respect
    /// to the given \c base type; aliases must be unique beneath that
    /// base type.
    ///
    TF_API
    void AddAlias(TfType base, const std::string &name) const;

    /// Convenience method to add an alias and return *this.
    /// \see AddAlias()
    const TfType &Alias(TfType base, const std::string &name) const {
        AddAlias(base, name);
        return *this;
    }

    /// @}


    /// \name Pointer casts
    /// @{

    /// Cast \c addr to the address corresponding to the type \c ancestor.
    ///
    /// (This is a dangerous function; there's probably a much better way to
    /// do whatever it is you're trying to do.)
    ///
    /// With multiple inheritance, you can't do a reinterpret_cast back to an
    /// ancestor type; this function figures out how to cast addr to the
    /// address corresponding to the type ancestor if in fact ancestor is
    /// really an ancestor of the type corresponding to \c *this.
    ///
    /// In order for this function to work correctly, \p addr must have been a
    /// pointer of type corresponding to \c *this, which was cast to void; and
    /// of course the type of \p ancestor must be an ancestor of the type of
    /// \c *this.
    ///
    /// \warning You are warned: this is deadly dangerous stuff, and you
    /// shouldn't be doing it!
    TF_API
    void* CastToAncestor(TfType ancestor, void* addr) const;

    const void* CastToAncestor(TfType ancestor,
                               const void* addr) const {
        return CastToAncestor(ancestor, const_cast<void*>(addr));
    }
    
    /// Cast \c addr, which pointed to the ancestor type \p ancestor, to the
    /// type of \c *this.
    ///
    /// This function is the opposite of \c CastToAncestor(); the assumption
    /// is that \c addr was a pointer to the type corresponding to \c
    /// ancestor, and was then reinterpret-cast to \c void*, but now you wish
    /// to turn cast the pointer to the type corresponding to \c *this.  While
    /// the fact that \p addr was a pointer of type \c ancestor is taken on
    /// faith, a runtime check is performed to verify that the underlying
    /// object pointed to by \p addr is of type \c *this (or derived from \c
    /// *this).
    ///
    /// \warning Again, this is dangerous territory, and there's probably
    /// something much better than using this function.
    TF_API
    void* CastFromAncestor(TfType ancestor, void* addr) const;

    const void* CastFromAncestor(TfType ancestor,
                                 const void* addr) const {
        return CastFromAncestor(ancestor, const_cast<void*>(addr));
    }

    /// @}

    /// \name Instantiation / Manufacturing
    /// @{

    /// Sets the factory object for this type.  A type's factory typically
    /// has methods to instantiate the type given various arguments and must
    /// inherit from \c FactoryBase.  The factory cannot be changed once set.
    TF_API
    void SetFactory(std::unique_ptr<FactoryBase> factory) const;

    /// Sets the factory object for this type.  A type's factory typically
    /// has methods to instantiate the type given various arguments and must
    /// inherit from \c FactoryBase.  The factory cannot be changed once set.
    template <class T>
    void SetFactory(std::unique_ptr<T>& factory) const {
        SetFactory(std::unique_ptr<FactoryBase>(std::move(factory)));
    }

    /// Sets the factory object for this type to be a \c T.  The factory
    /// cannot be changed once set.
    template <class T>
    void SetFactory() const { SetFactory(std::unique_ptr<FactoryBase>(new T)); }

    /// Sets the factory object for this type.  A type's factory typically
    /// has methods to instantiate the type given various arguments and must
    /// inherit from \c FactoryBase.  The factory cannot be changed once set.
    const TfType& Factory(std::unique_ptr<FactoryBase> factory) const {
        SetFactory(std::move(factory));
        return *this;
    }

    /// Sets the factory object for this type.  A type's factory typically
    /// has methods to instantiate the type given various arguments and must
    /// inherit from \c FactoryBase.  The factory cannot be changed once set.
    template <class T>
    const TfType& Factory(std::unique_ptr<T>& factory) const
    {
        SetFactory(std::unique_ptr<FactoryBase>(std::move(factory)));
        return *this;
    }

    /// Sets the factory object for this type to be a \c T.  The factory
    /// cannot be changed once set.
    template <class T>
    const TfType& Factory() const {
        SetFactory(std::unique_ptr<FactoryBase>(new T));
        return *this;
    }

    /// Returns the factory object for this type as a \c T*, or \c NULL if
    /// there is no factory or the factory is not or is not derived from \c T.
    /// Clients can check if a factory is set using
    /// \c GetFactory<TfType::FactoryBase>().
    template <class T>
    T *GetFactory() const { return dynamic_cast<T*>(_GetFactory()); }

    /// @}

private:
    TF_API
    FactoryBase* _GetFactory() const;

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TF_API
    static TfType const &_FindImplPyPolymorphic(PyPolymorphicBase const *ptr);
    
    // PyPolymorphic case.
    template <class T>
    static typename std::enable_if<
        std::is_base_of<PyPolymorphicBase, T>::value, TfType const &>::type
    _FindImpl(T const *rawPtr) {
        return _FindImplPyPolymorphic(
            static_cast<PyPolymorphicBase const *>(rawPtr));
    }

    // Polymorphic.
    template <class T>
    static typename std::enable_if<
        std::is_polymorphic<T>::value &&
        !std::is_base_of<PyPolymorphicBase, T>::value, TfType const &>::type
    _FindImpl(T const *rawPtr) {
        if (auto ptr = dynamic_cast<PyPolymorphicBase const *>(rawPtr))
            return _FindImplPyPolymorphic(ptr);
        return Find(typeid(*rawPtr));
    }

    template <class T>
    static typename std::enable_if<
        !std::is_polymorphic<T>::value, TfType const &>::type
    _FindImpl(T const *rawPtr) {
        return Find(typeid(T));
    }

#else
    template <class T>
    static typename std::enable_if<
        std::is_polymorphic<T>::value, TfType const &>::type
    _FindImpl(T const *rawPtr) {
        return Find(typeid(*rawPtr));
    }

    template <class T>
    static typename std::enable_if<
        !std::is_polymorphic<T>::value, TfType const &>::type
    _FindImpl(T const *rawPtr) {
        return Find(typeid(T));
    }

#endif // PXR_PYTHON_SUPPORT_ENABLED

    bool _IsAImpl(TfType queryType) const;

    typedef void *(*_CastFunction)(void *, bool derivedToBase);

    template <typename TypeVector>
    friend struct Tf_AddBases;
    friend struct _TypeInfo;
    friend class Tf_TypeRegistry;
    friend class TfHash;

    // Construct a TfType with the given _TypeInfo.
    explicit TfType(_TypeInfo *info) : _info(info) {}

    // Add a base type, and link as a derived type of that base.
    void _AddBase( TfType base ) const;

    // Add the given function for casting to/from the given baseType.
    TF_API
    void _AddCppCastFunc(
        const std::type_info &baseTypeInfo, _CastFunction) const;

    // Define this TfType to have the given type_info.
    TF_API
    void _DefineCppType(const std::type_info &,
                        size_t sizeofType,
                        bool isPodType,
                        bool isEnumType) const;

    // Execute the definition callback if one exists.
    void _ExecuteDefinitionCallback() const;

    // Retrieve the \c TfType corresponding to an obj with the
    // given \c type_info.
    TF_API
    static TfType const& _FindByTypeid(const std::type_info &);

    // Pointer to internal type representation.
    // Our only data member.
    _TypeInfo *_info;
};

/// Output a TfType, using the machine-independent type name.
/// \ingroup group_tf_DebuggingOutput
TF_API std::ostream& operator<<(std::ostream& out, const TfType &t);

/// Metafunction returning sizeof(T) for a type T (or 0 if T is a void type).
template <typename T>
struct TfSizeofType {
    static const size_t value = sizeof(T);
};
template <>
struct TfSizeofType<void> {
    static const size_t value = 0;
};
template <>
struct TfSizeofType<const void> {
    static const size_t value = 0;
};
template <>
struct TfSizeofType<volatile void> {
    static const size_t value = 0;
};
template <>
struct TfSizeofType<const volatile void> {
    static const size_t value = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

// Implementation details are put in this header.
#include "pxr/base/tf/type_Impl.h"

#endif // TF_TYPE_H
