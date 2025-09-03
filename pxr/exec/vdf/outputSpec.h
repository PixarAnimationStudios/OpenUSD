//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_OUTPUT_SPEC_H
#define PXR_EXEC_VDF_OUTPUT_SPEC_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/typedVector.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

// Manually implemented vtable+typeinfo block for VdfOutputSpec.
struct Vdf_OutputSpecTypeInfo
{
    using AllocateCacheFn = VdfVector* (*)();
    using ResizeCacheFn = void (*)(VdfVector *, const VdfMask::Bits &);

    TfType type;
    AllocateCacheFn allocateCache;
    ResizeCacheFn resizeCache;
};

////////////////////////////////////////////////////////////////////////////////
///
/// A VdfOuptutSpec describes an output connector.  It stores typing
/// information and the connector's name.
///
class VdfOutputSpec
{
public:
    TF_MALLOC_TAG_NEW("Vdf", "new VdfOutputSpec");

    template <typename T>
    static VdfOutputSpec *New(const TfToken &name) {
        return new VdfOutputSpec(_GenerateTypeInfo<T>(), name);
    }

    VDF_API
    static VdfOutputSpec *New(TfType type, const TfToken &name);

    VDF_API
    ~VdfOutputSpec();

    /// Returns the name of this spec's type.
    VDF_API
    std::string GetTypeName() const;

    /// Returns the type of this spec
    TfType GetType() const { return _typeinfo->type; }

    /// Returns the name of this connector.
    const TfToken &GetName() const { return _name; }

    /// Allocate a new VdfVector with this spec's type.
    VDF_API
    VdfVector *AllocateCache() const;

    /// Resize an existing VdfVector to accomodate all the data set in the bits.
    VDF_API
    void ResizeCache(VdfVector *vector, const VdfMask::Bits &bits) const;

    /// Returns true, if two output specs are equal.
    ///
    bool operator==(const VdfOutputSpec &rhs) const {
        return GetType() == rhs.GetType() && _name == rhs._name;
    }
    bool operator!=(const VdfOutputSpec &rhs) const {
        return !(*this == rhs);
    }

    /// Returns a hash for this instance.
    VDF_API
    size_t GetHash() const;

private:

    VdfOutputSpec(
        const Vdf_OutputSpecTypeInfo *typeinfo,
        const TfToken &name)
        : _typeinfo(typeinfo)
        , _name(name)
    {}

    template <typename T>
    static VdfVector *_AllocateCache() {
        return new VdfTypedVector<T>();
    }

    template <typename T>
    static void _ResizeCache(VdfVector *cache, const VdfMask::Bits &bits) {
        cache->Resize<T>(bits);
    }

    // Return a pointer to a static vtable+typeinfo block.
    template <typename T>
    static const Vdf_OutputSpecTypeInfo * _GenerateTypeInfo() {
        static const Vdf_OutputSpecTypeInfo ti = {
            TfType::Find<T>(), _AllocateCache<T>, _ResizeCache<T>
        };
        return &ti;
    }

    // Register a type for runtime manufacturing.
    // Only VdfExecutionTypeRegistry should register types.
    friend class VdfExecutionTypeRegistry;
    VDF_API
    static void _RegisterType(const Vdf_OutputSpecTypeInfo *);

    template <typename T>
    static void _RegisterType() {
        _RegisterType(_GenerateTypeInfo<T>());
    }

private:
    const Vdf_OutputSpecTypeInfo *_typeinfo;
    TfToken _name;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
