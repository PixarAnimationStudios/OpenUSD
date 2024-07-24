//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDF_CHANGE_MANAGER_H
#define PXR_USD_SDF_CHANGE_MANAGER_H

/// \file sdf/changeManager.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/changeList.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/base/tf/singleton.h"

#include <tbb/enumerable_thread_specific.h>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

class SdfChangeBlock;
class SdfSpec;

/// \class Sdf_ChangeManager
///
/// Pathway for invalidation and change notification emitted by Sdf.
///
/// Since Sdf is the base representation in our system, and doesn't have
/// many derived computations, this primarily just queues up invalidation
/// notifications directly.
///
/// For now this class uses TfNotices to represent invalidations.
///
class Sdf_ChangeManager {
    Sdf_ChangeManager(const Sdf_ChangeManager&) = delete;
    Sdf_ChangeManager& operator=(const Sdf_ChangeManager&) = delete;
public:
    SDF_API
    static Sdf_ChangeManager& Get() {
        return TfSingleton<Sdf_ChangeManager>::GetInstance();
    }

    // Queue notifications.
    void DidReplaceLayerContent(const SdfLayerHandle &layer);
    void DidReloadLayerContent(const SdfLayerHandle &layer);
    void DidChangeLayerIdentifier(const SdfLayerHandle &layer,
                                  const std::string &oldIdentifier);
    void DidChangeLayerResolvedPath(const SdfLayerHandle &layer);
    void DidChangeField(const SdfLayerHandle &layer,
                        const SdfPath & path, const TfToken &field,
                        VtValue && oldValue, const VtValue & newValue );
    void DidChangeAttributeTimeSamples(const SdfLayerHandle &layer,
                                       const SdfPath &attrPath);

    // Spec changes.
    void DidMoveSpec(const SdfLayerHandle &layer,
                     const SdfPath & oldPath, const SdfPath & newPath);
    void DidAddSpec(const SdfLayerHandle &layer, const SdfPath &path, 
                    bool inert);
    void DidRemoveSpec(const SdfLayerHandle &layer, const SdfPath &path,
                       bool inert);
    void RemoveSpecIfInert(const SdfSpec&);

private:
    friend class SdfChangeBlock;
    
    struct _Data {
        _Data();
        SdfLayerChangeListVec changes;
        SdfChangeBlock const *outermostBlock;
        std::vector<SdfSpec> removeIfInert;
    };

    Sdf_ChangeManager();
    ~Sdf_ChangeManager();

    // Open a change block, and return a non-null pointer if this was the
    // outermost change block.  The caller must only call _CloseChangeBlock if
    // _OpenChangeBlock returned a non-null pointer, and pass it back.
    SDF_API
    void const *_OpenChangeBlock(SdfChangeBlock const *block);
    SDF_API
    void _CloseChangeBlock(SdfChangeBlock const *block, void const *openKey);

    void _SendNoticesForChangeList( const SdfLayerHandle & layer,
                                    const SdfChangeList & changeList );
    void _SendNotices(_Data *data);

    void _ProcessRemoveIfInert(_Data *data);

    SdfChangeList &_GetListFor(SdfLayerChangeListVec &changeList,
                               SdfLayerHandle const &layer);

private:

    tbb::enumerable_thread_specific<_Data> _data;

    friend class TfSingleton<Sdf_ChangeManager>;
};

SDF_API_TEMPLATE_CLASS(TfSingleton<Sdf_ChangeManager>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDF_CHANGE_MANAGER_H
