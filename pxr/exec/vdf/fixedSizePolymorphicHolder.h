//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_FIXED_SIZE_POLYMORPHIC_HOLDER_H
#define PXR_EXEC_VDF_FIXED_SIZE_POLYMORPHIC_HOLDER_H

#include "pxr/pxr.h"

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <utility>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class Vdf_FixedSizePolymorphicHolder
///
/// Used to implement small object optimizations for the type-erasure (Any)
/// pattern.
///
/// This class allows clients to instantiate polymorphic objects into a fixed
/// buffer space. If code attempts to instantiate an object that is too large to
/// fit in the allotted space, that code will generate a compile-time error.
/// Additionally, any instance of a derived class stored in this holder must
/// have its \p Base object at the same address. Practically speaking, this
/// means that multiple inheritance isn't supported. Currently this requirement
/// is enforced only at runtime.
///
template <class Base, size_t BufferSize>
class Vdf_FixedSizePolymorphicHolder
{
    static_assert(std::is_polymorphic<Base>::value, "");

    // The storage used to contain instances.
    using _Storage = typename std::aligned_storage_t<BufferSize, sizeof(void *)>;

public:

    // Noncopyable.
    Vdf_FixedSizePolymorphicHolder(
        Vdf_FixedSizePolymorphicHolder const &) = delete;
    Vdf_FixedSizePolymorphicHolder &
    operator=(Vdf_FixedSizePolymorphicHolder const &) = delete;

    Vdf_FixedSizePolymorphicHolder() = default;

    /// Creates an instance
    ///
    /// Uses placement-new to create an object of type \p Derived into the local
    /// storage. Will fail to compile if \p Derived's size or alignment is
    /// incompatible with the storage.
    template <class Derived, class ... Args>
    void New(Args && ... args) {
        static_assert(
            std::is_base_of<Base, Derived>::value,
            "Derived is not a derived class of Base.");
        static_assert(
            sizeof(Derived) <= sizeof(_Storage),
            "The size of the derived type is larger than the availble storage.");
        static_assert(
            alignof(_Storage) % alignof(Derived) == 0,
            "The derived type has incompatible alignment.");
        new (_GetStorage()) Derived(std::forward<Args>(args)...);
        // Verification that Base and Derived start at the same address.
        // Perhaps this can be done at compile-time?
        void *const bStart =
            static_cast<Base *>(static_cast<Derived *>(_GetStorage()));
        TF_AXIOM(bStart == _GetStorage());
    }

    /// Destroys a held instance
    ///
    /// Invokes `~Base` on the held object. A valid instance of type Base or a
    /// type derived from Base must already be held.
    void Destroy() {
#if defined(ARCH_COMPILER_GCC) && ARCH_COMPILER_GCC_MAJOR < 11        
        ARCH_PRAGMA_PUSH
        ARCH_PRAGMA_MAYBE_UNINITIALIZED
#endif
        Get()->~Base();
#if defined(ARCH_COMPILER_GCC) && ARCH_COMPILER_GCC_MAJOR < 11
        ARCH_PRAGMA_POP
#endif 
    }

    /// Returns a Base pointer to the held instance.
    inline Base const *Get() const {
        return static_cast<Base const *>(_GetStorage());
    }
    
    /// Returns a Base pointer to the held instance.
    inline Base *Get() {
        return static_cast<Base *>(_GetStorage());
    }
    
private:

    // Returns a pointer to the instance storage space.
    inline void *_GetStorage() {
        return &_storage;
    }
    
    // Returns a pointer to the instance storage space.
    inline void const *_GetStorage() const {
        return &_storage;
    }


    //
    // Data Members
    //

    _Storage _storage;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
