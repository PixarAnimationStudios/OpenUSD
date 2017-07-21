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
#ifndef SDF_CHANGEMANAGER_H
#define SDF_CHANGEMANAGER_H

/// \file sdf/changeManager.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/changeList.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/spec.h"
#include "pxr/base/tf/singleton.h"

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <tbb/enumerable_thread_specific.h>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

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
class Sdf_ChangeManager : boost::noncopyable {
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
                        const VtValue & oldValue, const VtValue & newValue );
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

    // Open/close change blocks. SdfChangeBlock provides stack-based management
    // of change blocks and should be preferred over this API.
    SDF_API
    void OpenChangeBlock();
    SDF_API
    void CloseChangeBlock();

private:
    Sdf_ChangeManager();
    ~Sdf_ChangeManager();

    void _SendNoticesForChangeList( const SdfLayerHandle & layer,
                                    const SdfChangeList & changeList );
    void _SendNotices();

    void _ProcessRemoveIfInert();

private:
    struct _Data {
        _Data();
        SdfLayerChangeListMap changes;
        int changeBlockDepth;
        std::vector<SdfSpec> removeIfInert;
    };

    tbb::enumerable_thread_specific<_Data> _data;

    friend class TfSingleton<Sdf_ChangeManager>;
};

SDF_API_TEMPLATE_CLASS(TfSingleton<Sdf_ChangeManager>);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_CHANGEMANAGER_H
