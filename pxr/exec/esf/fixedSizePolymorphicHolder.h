//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_FIXED_SIZE_POLYMORPHIC_HOLDER_H
#define PXR_EXEC_ESF_FIXED_SIZE_POLYMORPHIC_HOLDER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/api.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/tf.h"

#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Enables a base class to be used with EsfFixedSizePolymorphicHolder.
///
/// This class defines private virtual methods that are used internally by
/// EsfFixedSizePolymorphicHolder. Clients must not override these methods.
///
class ESF_API_TYPE EsfFixedSizePolymorphicBase
{
public:
    ESF_API virtual ~EsfFixedSizePolymorphicBase();

private:
    template <class Base, size_t BufferSize>
    friend class EsfFixedSizePolymorphicHolder;

    // Copies this object to the storage of another
    // EsfFixedSizePolymorphicHolder.
    //
    // This class provides a default implementation so that derived types can be
    // instantiated without having to override it. The proper overridden
    // implementation will be defined by EsfFixedSizePolymorphicHolder.
    //
    ESF_API virtual void _CopyTo(std::byte *storage) const;

    // Moves this object to the storage of another
    // EsfFixedSizePolymorphicHolder.
    //
    // This class provides a default implementation so that derived types can be
    // instantiated without having to override it. The proper overridden
    // implementation will be defined by EsfFixedSizePolymorphicHolder.
    //
    ESF_API virtual void _MoveTo(std::byte *storage);
};

/// Stores polymorphic objects in a fixed-size buffer.
///
/// If code attempts to instantiate an object that is too large to
/// fit in the allotted space, that code will generate a compile-time error.
/// Additionally, any instance of a derived class stored in this holder must
/// have its \p Base object at the same address. Currently this requirement is
/// enforced only at runtime.
///
/// \p Base itself must inherit from EsfFixedSizePolymorphicBase.
///
/// Instances of this class *always* contain a derived object. This class
/// supports copy/move construction and copy/move assignment, so long as the
/// derived type held by the "source" holder also supports copy/move
/// construction. Copying/moving of EsfFixedSizePolymorphicHolder%s is only
/// supported between identical template specializations (both must have
/// the same \p Base and \p BufferSize).
///
template <class Base, size_t BufferSize = sizeof(Base)>
class EsfFixedSizePolymorphicHolder
{
    static_assert(std::is_base_of_v<EsfFixedSizePolymorphicBase, Base>);
    constexpr static size_t _REQUIRED_ALIGNMENT = alignof(Base);

public:
    using This = EsfFixedSizePolymorphicHolder<Base, BufferSize>;

    /// The default constructor is deleted because instances must *always*
    /// contain a derived object.
    ///
    EsfFixedSizePolymorphicHolder() = delete;

    /// Construct a holder emplaced with a new \p Derived instance.
    ///
    /// The \p Derived type is deduced from the \p derivedTypeHint parameter,
    /// usually passed as `std::in_place_type<Derived>`. This is necessary
    /// because constructors cannot be invoked with explicit template arguments.
    ///
    /// The \p Args are forwarded to the \p Derived instance's constructor.
    ///
    template <class Derived, class... Args>
    EsfFixedSizePolymorphicHolder(
        std::in_place_type_t<Derived> derivedTypeHint, Args &&...args);

    /// Construct a holder containing a derived type instance that is copy-
    /// constructed from another holder.
    ///
    EsfFixedSizePolymorphicHolder(const This &other) {
        other->_CopyTo(_storage);
    }

    /// Construct a holder containing a derived type that is move-constructed
    /// from another holder.
    ///
    /// The moved-from holder continues to hold an object, but that object is
    /// moved-from.
    ///
    EsfFixedSizePolymorphicHolder(This &&other) {
        other->_MoveTo(_storage);
    }

    /// Destroys the derived instance held by this object.
    ///
    ~EsfFixedSizePolymorphicHolder() {
        Get()->~Base();
    }

    /// Construct a new instance that is copy-constructed from the instance in
    /// another holder.
    ///
    /// This holder destroys its current instance before constructing the
    /// new instance.
    ///
    This &operator=(const This &other) {
        if (this != &other) {
            Get()->~Base();
            other->_CopyTo(_storage);
        }
        return *this;
    }

    /// Construct a new instance that is move-constructed from the instance in
    /// another holder.
    ///
    /// This holder destroys its current instance before constructing the new
    /// instance. The moved-from holder continues to hold an object, but that
    /// object is moved-from.
    ///
    This &operator=(This &&other) {
        if (this != &other) {
            Get()->~Base();
            other->_MoveTo(_storage);
        }
        return *this;
    }

    /// \name Held object accessors
    ///
    /// Returns a pointer or reference to the held Base instance.
    ///
    /// \{
    inline Base *Get() {
        return reinterpret_cast<Base *>(_storage);
    }

    inline const Base *Get() const {
        return reinterpret_cast<const Base *>(_storage);
    }

    inline Base *operator->() {
        return Get();
    }

    inline const Base *operator->() const {
        return Get();
    }

    inline Base &operator*() {
        return *Get();
    }

    inline const Base &operator*() const {
        return *Get();
    }
    /// \}

    /// Checks if instances of \p Derived can be stored in this holder.
    ///
    /// If any member of this class is false, then instances of \p Derived
    /// cannot be stored in this holder. These checks are made public for
    /// testing.
    ///
    struct Compatibility
    {
        template <class Derived>
        constexpr static bool FITS_IN_BUFFER = sizeof(Derived) <= BufferSize;
        
        template <class Derived>
        constexpr static bool DERIVES_FROM_BASE =
            std::is_base_of_v<Base, Derived>;
        
        template <class Derived>
        constexpr static bool HAS_ALIGNMENT =
            alignof(Derived) == _REQUIRED_ALIGNMENT;

        /// Verifies that a Derived instance has the same address as its Base.
        ///
        /// This check would fail for certain cases of multiple-inheritance.
        /// C++ does not currently allow checking this at compile-time, so this
        /// check can only happen at runtime.
        ///
        template <class Derived>
        static bool HasBaseAtSameAddress(const Derived &derived);
    };

private:
    // Implements the _CopyTo and _MoveTo methods for a derived type.
    template <class Derived>
    class _Holder : public Derived
    {
    public:
        // Constructs a Derived instance from the provided constructor Args.
        template <class... Args>
        _Holder(Args &&...args) : Derived(std::forward<Args>(args)...) {
            TF_AXIOM(Compatibility::HasBaseAtSameAddress(*this));
        }

    private:
        void _CopyTo(std::byte *storage) const final {
            const Derived *thisDerived = static_cast<const Derived *>(this);
            ::new (static_cast<void *>(storage)) _Holder<Derived>(*thisDerived);
        }

        void _MoveTo(std::byte *storage) final {
            Derived *thisDerived = static_cast<Derived *>(this);
            ::new (static_cast<void *>(storage)) _Holder<Derived>(
                std::move(*thisDerived));
        }
    };

    // Held instances are emplaced in this byte array.
    alignas(_REQUIRED_ALIGNMENT) std::byte _storage[BufferSize];
};

template <class Base, size_t BufferSize>
template <class Derived, class... Args>
EsfFixedSizePolymorphicHolder<Base, BufferSize>::EsfFixedSizePolymorphicHolder(
    std::in_place_type_t<Derived> derivedTypeHint, Args &&...args)
{
    TF_UNUSED(derivedTypeHint);

    static_assert(
        Compatibility::template FITS_IN_BUFFER<Derived>,
        "The size of the derived type is larger than the availble "
        "storage.");
    static_assert(
        Compatibility::template DERIVES_FROM_BASE<Derived>,
        "Derived is not a derived class of Base.");
    static_assert(
        Compatibility::template HAS_ALIGNMENT<Derived>,
        "The derived type has incompatible alignment.");

    ::new (static_cast<void *>(_storage)) _Holder<Derived>(
        std::forward<Args>(args)...);
}

template <class Base, size_t BufferSize>
template <class Derived>
bool
EsfFixedSizePolymorphicHolder<Base, BufferSize>::Compatibility::
    HasBaseAtSameAddress(const Derived &derived)
{
    if constexpr (!DERIVES_FROM_BASE<Derived>) {
        return false;
    }

    const Base *base = static_cast<const Base *>(&derived);
    const void *baseAddress = static_cast<const void *>(base);
    const void *derivedAddress = static_cast<const void *>(&derived);
    return baseAddress == derivedAddress;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
