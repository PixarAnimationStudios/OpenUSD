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
#ifndef SDF_IDENTITY_H
#define SDF_IDENTITY_H

#include "pxr/pxr.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/usd/sdf/api.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

#include <boost/intrusive_ptr.hpp>
#include <tbb/spin_mutex.h>

PXR_NAMESPACE_OPEN_SCOPE

class Sdf_IdentityRegistry;
SDF_DECLARE_HANDLES(SdfLayer);

/// \class Sdf_Identity
///
/// Identifies the logical object behind an SdfSpec.
///
/// This is simply the layer the spec belongs to and the path to the spec.
///
class Sdf_Identity {
public:
    // Disallow copies
    Sdf_Identity(const Sdf_Identity&) = delete;
    Sdf_Identity& operator=(const Sdf_Identity&) = delete;

    /// Returns the layer that this identity refers to.
    SDF_API
    const SdfLayerHandle &GetLayer() const;

    /// Returns the path that this identity refers to.
    SDF_API
    const SdfPath &GetPath() const;

private:
    // Ref-counting ops manage _refCount.
    friend void intrusive_ptr_add_ref(Sdf_Identity*);
    friend void intrusive_ptr_release(Sdf_Identity*);

    friend class Sdf_IdentityRegistry;

    SDF_API
    Sdf_Identity(Sdf_IdentityRegistry *registry, const SdfPath &path);
    SDF_API
    ~Sdf_Identity();

    void _Forget();

    mutable std::atomic_int _refCount;
    Sdf_IdentityRegistry *_registry;
    SdfPath _path;
};

// Specialize boost::intrusive_ptr operations.
inline void intrusive_ptr_add_ref(PXR_NS::Sdf_Identity* p) {
    ++p->_refCount;
}
inline void intrusive_ptr_release(PXR_NS::Sdf_Identity* p) {
    if (--p->_refCount == 0) {
        delete p;
    }
}

class Sdf_IdentityRegistry : public boost::noncopyable {
public:
    Sdf_IdentityRegistry(const SdfLayerHandle &layer);
    ~Sdf_IdentityRegistry();

    /// Returns the layer that owns this registry.
    const SdfLayerHandle &GetLayer() const;

    /// Return the identity associated with \a path, issuing a new
    /// one if necessary. The registry will track the identity
    /// and update it if the logical object it represents moves
    /// in namespace.
    Sdf_IdentityRefPtr Identify(const SdfPath &path);

    /// Update identity in response to a namespace edit.
    void MoveIdentity(const SdfPath &oldPath, const SdfPath &newPath);
    
private:
    // When an identity expires, it will remove itself from the registry.
    friend class Sdf_Identity;

    // Remove the identity mapping for \a path to \a id from the registry.
    // This is only called by Sdf_Identity's destructor.
    void _Remove(const SdfPath &path, Sdf_Identity *id);

    /// The layer that owns this registry, and on behalf of which
    /// this registry tracks identities.
    const SdfLayerHandle _layer;
    
    /// The identities being managed by this registry
    typedef TfHashMap<SdfPath, Sdf_Identity*, SdfPath::Hash> _IdMap;
    _IdMap _ids;

    // This mutex synchronizes access to _ids.
    tbb::spin_mutex _idsMutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_IDENTITY_H
