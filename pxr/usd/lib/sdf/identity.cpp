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
#include "pxr/usd/sdf/identity.h"

//
// Sdf_Identity
//

Sdf_Identity::Sdf_Identity(Sdf_IdentityRegistry *registry,
    const SdfPath &path) :
    _refCount(0), _registry(registry), _path(path)
{
}

Sdf_Identity::~Sdf_Identity()
{
    if (_registry) {
        _registry->_Remove(_path, this);
    }
}

const SdfLayerHandle &
Sdf_Identity::GetLayer() const
{
    if (ARCH_LIKELY(_registry)) {
        return _registry->GetLayer();
    }

    static const SdfLayerHandle empty;
    return empty;
}

const SdfPath &
Sdf_Identity::GetPath() const
{
    return _path;
}

void Sdf_Identity::_Forget()
{
    _path = SdfPath();
    _registry = NULL;
}

//
// Sdf_IdentityRegistry
//

Sdf_IdentityRegistry::Sdf_IdentityRegistry(const SdfLayerHandle &layer) :
    _layer(layer)
{
}

Sdf_IdentityRegistry::~Sdf_IdentityRegistry()
{
    tbb::spin_mutex::scoped_lock lock(_idsMutex);

    TF_FOR_ALL(i, _ids) {
        (*i).second->_Forget();
    }
}

const SdfLayerHandle &
Sdf_IdentityRegistry::GetLayer() const
{
    return _layer;
}

Sdf_IdentityRefPtr
Sdf_IdentityRegistry::Identify(const SdfPath &path)
{
    tbb::spin_mutex::scoped_lock lock(_idsMutex);

    _IdMap::iterator i = _ids.find(path);
    if (i != _ids.end()) {
        Sdf_Identity *rawId = i->second;
        // Acquire an additional reference to this identity.  We need to do
        // this before proceeding to protect ourselves from race conditions,
        // since other threads could drop the ref-count of this identity at
        // any time, potentially beginning its destruction.
        if (rawId->_refCount++ > 0) {
            // The node is still in active use and we can share it.
            // Since we just acquired a reference here, we know the
            // node cannot expire before we return it.
            return Sdf_IdentityRefPtr(rawId, /* add_ref = */ false);
        } else {
            // The identity has expired but not yet been removed from
            // the registry map, due to the registry destructor racing
            // this function for the _idsMutex.
            //
            // We cannot re-use the identity because we cannot stop the
            // destructor from completing (and its memory being freed)
            // so we must allocate a new identity.  Discard the reference
            // we just acquired.
            --rawId->_refCount;
        }
    }

    TfAutoMallocTag2 tag("Sdf", "Sdf_IdentityRegistry::Identify");
    
    Sdf_Identity *id = new Sdf_Identity(this, path);

    // Note, this potentially overwrites an existing identity for this
    // path.  Per the code above, this only happens when the existing
    // identity is in the process of being destroyed.
    _ids[path] = id;

    return Sdf_IdentityRefPtr(id);
}

void
Sdf_IdentityRegistry::_Remove(const SdfPath &path, Sdf_Identity *id)
{
    tbb::spin_mutex::scoped_lock lock(_idsMutex);

    _IdMap::iterator i = _ids.find(path);

    if (i == _ids.end()) {
        // It is possible for this path entry to have already been
        // removed.  Consider the case where Identify() is called for
        // a path whose prior identity is expiring, but has not yet
        // been removed from the table (due to races bewteen threads).
        // Identify() will allocate a new identity at the same path.
        // If that new identity is then dropped, it can remove the
        // path from the table, all before the original identity
        // can be removed.
        //
        // An alternate design that might be cleaner would be to
        // do something like Sdf_PathNodes, where we allow multiple
        // expired entries to exist for a given path in addition
        // to at most one live one.
        return;
    }

    if (i->second == id) {
        // Only erase this entry if it still maps to this identity.
        // As described above, it is possible that Identify() has
        // replaced this with a new identity.
        _ids.erase(i);
    }
}

void 
Sdf_IdentityRegistry::MoveIdentity(const SdfPath &oldPath, 
                                   const SdfPath &newPath)
{
    // We hold the mutex, but note that per our Sdf thread-safety rules,
    // no other thread is allowed to be reading or writing this layer
    // at the same time that the layer is being mutated.
    tbb::spin_mutex::scoped_lock lock(_idsMutex);

    // Make sure an identity actually exists at the old path, otherwise
    // there's nothing to do.
    if (_ids.count(oldPath) == 0) {
        return;
    }

    // Insert an entry in the identity map for the new path. If an identity
    // already exists there, make sure we stomp it first.
    std::pair<_IdMap::iterator, bool> newIdStatus = 
        _ids.insert(std::make_pair(newPath, (Sdf_Identity*)NULL));
    if (not newIdStatus.second) {
        if (TF_VERIFY(newIdStatus.first->second)) {
            newIdStatus.first->second->_Forget();
        }
    }

    // Copy the identity from the entry at the old path to the new path and
    // update it to point at the new path.
    _IdMap::iterator oldIdIt = _ids.find(oldPath);
    newIdStatus.first->second = oldIdIt->second;
    newIdStatus.first->second->_path = newPath;
    
    // Erase the old identity map entry.
    _ids.erase(oldIdIt);
}
