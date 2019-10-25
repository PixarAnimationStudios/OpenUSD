//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYAGL_INSTANCER_IMAGER_H
#define PXRUSDMAYAGL_INSTANCER_IMAGER_H

/// \file pxrUsdMayaGL/instancerImager.h

#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/instancerShapeAdapter.h"

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/weakBase.h"

#include "usdMaya/notice.h"
#include "usdMaya/util.h"

#include <maya/MCallbackIdArray.h>
#include <maya/MDagMessage.h>

PXR_NAMESPACE_OPEN_SCOPE

/// Class for syncing native Maya instancers with the pxrHdImagingShape so that
/// it can draw USD reference assemblies connected to native Maya instancers.
///
/// XXX We currently don't support drawing multiple instanced instancers.
/// When instancer nodes appear at multiple points in the DAG path via native
/// Maya instancing, we only draw the 0th instance. This behavior is similar to
/// the current imaging behavior of USD proxy shapes, where only one instance
/// gets drawn by the draw override.
class UsdMayaGL_InstancerImager : public TfWeakBase {
public:
    static UsdMayaGL_InstancerImager& GetInstance();

    /// Sync all dirty instancer shape adapters for Viewport 2.0.
    /// If the shape adapters do not yet exist, they will be created.
    void SyncShapeAdapters(const unsigned int displayStyle);

    /// Sync all dirty instancer shape adapters for Legacy Viewport.
    /// If the shape adapters do not yet exist, they will be created.
    void SyncShapeAdapters(const M3dView::DisplayStyle legacyDisplayStyle);

    /// Destroys all shape adapters for currently tracked instancers, but
    /// does not stop tracking the instancers.
    /// Calling SyncShapeAdapters() again after this will recreate the shape
    /// adapters.
    /// If \p vp2 is set, destroys the VP2 adapters. Otherwise, destroys the
    /// Legacy Viewport adapters.
    void RemoveShapeAdapters(bool vp2);

private:
    /// Helper struct that owns all the data needed to track and draw a
    /// particular instancer node.
    struct _InstancerEntry {
        MCallbackIdArray callbacks;

        // The shape adapter generates an in-memory USD stage, so don't create
        // the shape adapters until we need them. For example, we might never
        // need the legacy shape adapter if we only have VP2 viewports.
        std::unique_ptr<UsdMayaGL_InstancerShapeAdapter> adapterVp2;
        std::unique_ptr<UsdMayaGL_InstancerShapeAdapter> adapterLegacy;

        ~_InstancerEntry();
    };

    /// Master list of all instancers being tracked.
    UsdMayaUtil::MObjectHandleUnorderedMap<_InstancerEntry> _instancers;

    /// List of instancers queued for removal. Won't be removed immediately,
    /// but will be removed on the next _StopTrackingInstancersToRemove().
    UsdMayaUtil::MObjectHandleUnorderedSet _instancersToRemove;

    /// Instancers that need a sync of their prototypes or instance data.
    UsdMayaUtil::MObjectHandleUnorderedSet _dirtyInstancersVp2;
    UsdMayaUtil::MObjectHandleUnorderedSet _dirtyInstancersLegacy;

    /// Instancers that need a sync of their world-space xform.
    UsdMayaUtil::MObjectHandleUnorderedSet _dirtyInstancerXformsVp2;
    UsdMayaUtil::MObjectHandleUnorderedSet _dirtyInstancerXformsLegacy;

    /// Cached handle to the global, singleton pxrHdImagingShape.
    MObjectHandle _cachedHdImagingShape;

    /// \name Maya MMessage callbacks (statics)
    /// @{

    /// Maya callback for when the given \p node becomes dirty.
    static void _OnNodeDirty(
            MObject& node,
            void* clientData);

    /// Maya callback for when the \p transformNode's world-space xform changes.
    /// \p transformNode is either the node for which the callback was
    /// registered or one of its ancestors.
    static void _OnWorldMatrixChanged(
            MObject &transformNode,
            MDagMessage::MatrixModifiedFlags& modified,
            void *clientData);

    /// @}
    /// \name Helpers
    /// @{

    /// Helper method to sync shape adapters for any instancers marked as dirty;
    /// this handles differences between VP2 and Legacy Viewport.
    void _SyncShapeAdapters(
            bool vp2,
            const unsigned int vp2DisplayStyle,
            const M3dView::DisplayStyle legacyDisplayStyle);

    /// Marks the global pxrHdImagingShape as dirty.
    /// If \p createIfNeeded is true, then creates the pxrHdImagingShape if it
    /// doesn't exist, and then marks it dirty. Otherwise, only dirties the
    /// shape if it already exists.
    void _DirtyHdImagingShape(bool createIfNeeded);

    /// @}
    /// \name Native instancer node management
    /// @{

    /// Adds an entry for the given instancer.
    void _StartTrackingInstancer(const MObject& instancer);
    /// Removes the entry for the given instancer.
    void _StopTrackingInstancer(const MObjectHandle& instancerHandle);

    /// @}
    /// \name Notice listeners (instance methods)
    /// @{

    /// Notice listener method for when the Maya scene resets.
    void _OnSceneReset(
            const UsdMayaSceneResetNotice& notice);
    /// Notice listener method for assembly-instancer connections.
    void _OnConnection(
            const UsdMayaAssemblyConnectedToInstancerNotice& notice);
    /// Notice listener method for assembly-instancer disconnections.
    void _OnDisconnection(
            const UsdMayaAssemblyDisconnectedFromInstancerNotice& notice);

    /// @}

    UsdMayaGL_InstancerImager();
    ~UsdMayaGL_InstancerImager();

    friend class TfSingleton<UsdMayaGL_InstancerImager>;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
