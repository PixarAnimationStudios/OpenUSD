//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/identity.h"
#include "pxr/base/tf/pxrTslRobinMap/robin_map.h"

#include <tbb/spin_mutex.h>

PXR_NAMESPACE_OPEN_SCOPE

//
// Sdf_IdRegistryImpl
//

static const size_t _MinDeadThreshold = 64;

class Sdf_IdRegistryImpl
{
public:
    explicit Sdf_IdRegistryImpl(SdfLayerHandle const &layer)
        : _layer(layer)
        , _deadCount(0)
        , _deadThreshold(_MinDeadThreshold) {}
    
    ~Sdf_IdRegistryImpl() {
        tbb::spin_mutex::scoped_lock lock(_idsMutex);

        for (auto &id: _ids) {
            id.second->_Forget();
        }
    }

    SdfLayerHandle const &GetLayer() const {
        return _layer;
    }

    Sdf_IdentityRefPtr
    Identify(const SdfPath &path) {
        
        tbb::spin_mutex::scoped_lock lock(_idsMutex);

        Sdf_Identity *rawId;
        _IdMap::iterator iter = _ids.find(path);
        if (iter != _ids.end()) {
            rawId = iter->second;
            ++rawId->_refCount;
            return Sdf_IdentityRefPtr(
                TfDelegatedCountDoNotIncrementTag, rawId);
        }

        TfAutoMallocTag2 tag("Sdf", "Sdf_IdentityRegistry::Identify");
        rawId = new Sdf_Identity(this, path);
        _ids[path] = rawId;
        _deadThreshold = std::max(_MinDeadThreshold, _ids.size() / 8);
        return Sdf_IdentityRefPtr(TfDelegatedCountIncrementTag, rawId);
    }

    void UnregisterOrDelete() {
        if (++_deadCount >= _deadThreshold) {
            // Clean house!
            _deadCount = 0;
            tbb::spin_mutex::scoped_lock lock(_idsMutex);
            for (auto iter = _ids.begin(); iter != _ids.end();) {

                if (iter->second->_refCount == 0) {
                    delete iter->second;
                    iter = _ids.erase(iter);
                }
                else {
                    ++iter;
                }
            }
            _deadThreshold = std::max(_MinDeadThreshold, _ids.size() / 8);
        }
    }

    void MoveIdentity(const SdfPath &oldPath, 
                      const SdfPath &newPath) {
        // We hold the mutex, but note that per our Sdf thread-safety rules, no
        // other thread is allowed to be reading or writing this layer at the
        // same time that the layer is being mutated.
        tbb::spin_mutex::scoped_lock lock(_idsMutex);
        
        // Make sure an identity actually exists at the old path, otherwise
        // there's nothing to do.
        if (_ids.count(oldPath) == 0) {
            return;
        }

        // Insert an entry in the identity map for the new path. If an identity
        // already exists there, make sure we stomp it first.
        std::pair<_IdMap::iterator, bool> newIdStatus = 
            _ids.insert(std::make_pair(newPath, (Sdf_Identity*)nullptr));
        if (!newIdStatus.second) {
            if (TF_VERIFY(newIdStatus.first->second)) {
                newIdStatus.first->second->_Forget();
            }
        }
        
        // Copy the identity from the entry at the old path to the new path and
        // update it to point at the new path.
        _IdMap::iterator oldIdIt = _ids.find(oldPath);
        newIdStatus.first.value() = oldIdIt->second;
        newIdStatus.first->second->_path = newPath;
        
        // Erase the old identity map entry.
        _ids.erase(oldIdIt);
    }

private:
    /// The identities being managed by this registry
    using _IdMap = pxr_tsl::robin_map<SdfPath, Sdf_Identity *, SdfPath::Hash>;
    _IdMap _ids;

    SdfLayerHandle _layer;

    /// A count of the number of dead identity objects in _ids, so we can clean
    /// it when it gets large.
    std::atomic<size_t> _deadCount;
    size_t _deadThreshold;

    // This mutex synchronizes access to _ids.
    tbb::spin_mutex _idsMutex;
};

//
// Sdf_Identity
//

const SdfLayerHandle &
Sdf_Identity::GetLayer() const
{
    if (ARCH_LIKELY(_regImpl)) {
        return _regImpl->GetLayer();
    }

    static const SdfLayerHandle empty;
    return empty;
}

void Sdf_Identity::_Forget()
{
    _path = SdfPath();
    _regImpl = nullptr;
}

void
Sdf_Identity::_UnregisterOrDelete(Sdf_IdRegistryImpl *regImpl,
                                  Sdf_Identity *id) noexcept
{
    if (regImpl) {
        regImpl->UnregisterOrDelete();
    }
    else {
        delete id;
    }
}


//
// Sdf_IdentityRegistry
//

Sdf_IdentityRegistry::Sdf_IdentityRegistry(const SdfLayerHandle &layer)
    : _layer(layer)
    , _impl(new Sdf_IdRegistryImpl(layer))
{
}

Sdf_IdentityRegistry::~Sdf_IdentityRegistry() = default;

Sdf_IdentityRefPtr
Sdf_IdentityRegistry::Identify(const SdfPath &path)
{
    return _impl->Identify(path);
}

void
Sdf_IdentityRegistry::MoveIdentity(
    const SdfPath &oldPath, const SdfPath &newPath)
{
    return _impl->MoveIdentity(oldPath, newPath);
}

void
Sdf_IdentityRegistry::_UnregisterOrDelete()
{
    _impl->UnregisterOrDelete();
}

PXR_NAMESPACE_CLOSE_SCOPE
