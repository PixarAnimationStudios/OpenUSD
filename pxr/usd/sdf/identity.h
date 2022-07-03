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
#ifndef PXR_USD_SDF_IDENTITY_H
#define PXR_USD_SDF_IDENTITY_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Sdf_IdentityRegistry;
class Sdf_IdRegistryImpl;

SDF_DECLARE_HANDLES(SdfLayer);

/// \class Sdf_Identity
///
/// Identifies the logical object behind an SdfSpec.
///
/// This is simply the layer the spec belongs to and the path to the spec.
///
class Sdf_Identity {
    Sdf_Identity(Sdf_Identity const &) = delete;
    Sdf_Identity &operator=(Sdf_Identity const &) = delete;
public:
    /// Returns the layer that this identity refers to.
    SDF_API
    const SdfLayerHandle &GetLayer() const;

    /// Returns the path that this identity refers to.
    const SdfPath &GetPath() const {
        return _path;
    }
    
private:
    // Ref-counting ops manage _refCount.
    friend void intrusive_ptr_add_ref(Sdf_Identity*);
    friend void intrusive_ptr_release(Sdf_Identity*);

    friend class Sdf_IdentityRegistry;
    friend class Sdf_IdRegistryImpl;

    Sdf_Identity(Sdf_IdRegistryImpl *regImpl, const SdfPath &path)
        : _refCount(0), _path(path), _regImpl(regImpl) {}
    
    SDF_API
    static void _UnregisterOrDelete(Sdf_IdRegistryImpl *reg, Sdf_Identity *id);
    void _Forget();

    mutable std::atomic_int _refCount;
    SdfPath _path;
    Sdf_IdRegistryImpl *_regImpl;
};

// Specialize boost::intrusive_ptr operations.
inline void intrusive_ptr_add_ref(PXR_NS::Sdf_Identity* p) {
    ++p->_refCount;
}
inline void intrusive_ptr_release(PXR_NS::Sdf_Identity* p) {
    // Once the count hits zero, p is liable to be destroyed at any point,
    // concurrently, by its owning registry if it happens to be doing a cleanup
    // pass.  Cache 'this' and the impl ptr in local variables so we have them
    // before decrementing the count.
    Sdf_Identity *self = p;
    Sdf_IdRegistryImpl *reg = p->_regImpl;
    if (--p->_refCount == 0) {
        // Cannot use 'p' anymore here.
        Sdf_Identity::_UnregisterOrDelete(reg, self);
    }
}

class Sdf_IdentityRegistry : public boost::noncopyable {
public:
    Sdf_IdentityRegistry(const SdfLayerHandle &layer);
    ~Sdf_IdentityRegistry();

    /// Returns the layer that owns this registry.
    const SdfLayerHandle &GetLayer() const {
        return _layer;
    }

    /// Return the identity associated with \a path, issuing a new
    /// one if necessary. The registry will track the identity
    /// and update it if the logical object it represents moves
    /// in namespace.
    Sdf_IdentityRefPtr Identify(const SdfPath &path);

    /// Update identity in response to a namespace edit.
    void MoveIdentity(const SdfPath &oldPath, const SdfPath &newPath);
    
private:
    friend class Sdf_Identity;

    friend class Sdf_IdRegistryImpl;

    // Remove the identity mapping for \a path to \a id from the registry.  This
    // is invoked when an identity's refcount hits zero.
    SDF_API
    void _UnregisterOrDelete();

    /// The layer that owns this registry, and on behalf of which
    /// this registry tracks identities.
    const SdfLayerHandle _layer;

    // Private implementation.
    const std::unique_ptr<Sdf_IdRegistryImpl> _impl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_IDENTITY_H
