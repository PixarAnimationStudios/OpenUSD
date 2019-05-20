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
#include "pxrUsdMayaGL/instancerImager.h"

#include "pxrUsdMayaGL/batchRenderer.h"
#include "pxrUsdMayaGL/debugCodes.h"

#include "usdMaya/hdImagingShape.h"
#include "usdMaya/referenceAssembly.h"

#include "pxr/base/tf/instantiateSingleton.h"

#include <maya/MDagMessage.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MGlobal.h>
#include <maya/MNodeMessage.h>
#include <maya/MObjectHandle.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_SINGLETON(UsdMayaGL_InstancerImager);

/* static */
UsdMayaGL_InstancerImager&
UsdMayaGL_InstancerImager::GetInstance()
{
    return TfSingleton<UsdMayaGL_InstancerImager>::GetInstance();
}

void
UsdMayaGL_InstancerImager::SyncShapeAdapters(const unsigned int displayStyle)
{
    // Viewport 2.0 sync.
    _SyncShapeAdapters(true, displayStyle, M3dView::DisplayStyle::kBoundingBox);
}

void
UsdMayaGL_InstancerImager::SyncShapeAdapters(
        const M3dView::DisplayStyle legacyDisplayStyle)
{
    // Legacy sync.
    _SyncShapeAdapters(false, 0, legacyDisplayStyle);
}

void
UsdMayaGL_InstancerImager::_SyncShapeAdapters(
        bool vp2,
        const unsigned int vp2DisplayStyle,
        const M3dView::DisplayStyle legacyDisplayStyle)
{
    // Clean up any instancers scheduled for deletion, and remove their shape
    // adapters.
    for (const MObjectHandle& handle : _instancersToRemove) {
        _StopTrackingInstancer(handle);
    }
    _instancersToRemove.clear();

    // Sync all of the shape adapters.
    // This will create the shape adapters if they don't yet exist.
    UsdMayaUtil::MObjectHandleUnorderedSet& dirtyInstancers =
            vp2 ? _dirtyInstancersVp2 : _dirtyInstancersLegacy;
    for (const MObjectHandle& handle : dirtyInstancers) {
        auto iter = _instancers.find(handle);
        if (iter == _instancers.end()) {
            continue;
        }

        if (!TF_VERIFY(handle.isAlive())) {
            // We should have removed this handle from all dirty lists before
            // it died. Clean it up now so that this doesn't happen again.
            _StopTrackingInstancer(handle);
            continue;
        }

        const MDagPath firstInstancePath =
                MDagPath::getAPathTo(handle.object());
    
        // Create the adapter if it doesn't exist yet.
        _InstancerEntry& entry = iter->second;
        if (vp2) {
            std::unique_ptr<UsdMayaGL_InstancerShapeAdapter>& adapter =
                    entry.adapterVp2;
            if (!adapter) {
                adapter.reset(new UsdMayaGL_InstancerShapeAdapter());
            }

            if (adapter->Sync(
                    firstInstancePath,
                    vp2DisplayStyle,
                    MHWRender::kDormant)) {
                UsdMayaGLBatchRenderer::GetInstance().AddShapeAdapter(
                        adapter.get());
            }
        }
        else {
            std::unique_ptr<UsdMayaGL_InstancerShapeAdapter>& adapter =
                    entry.adapterLegacy;
            if (!adapter) {
                adapter.reset(new UsdMayaGL_InstancerShapeAdapter());
            }

            if (adapter->Sync(
                    firstInstancePath,
                    legacyDisplayStyle,
                    M3dView::kDormant)) {
                UsdMayaGLBatchRenderer::GetInstance().AddShapeAdapter(
                        adapter.get());
            }
        }
    }
    dirtyInstancers.clear();

    // Sync all of the dirty root transforms now.
    // The shape adapters should already have been created above.
    UsdMayaUtil::MObjectHandleUnorderedSet& dirtyInstancerXforms =
            vp2 ? _dirtyInstancerXformsVp2 : _dirtyInstancerXformsLegacy;
    for (const MObjectHandle& handle : dirtyInstancerXforms) {
        auto iter = _instancers.find(handle);
        if (iter == _instancers.end()) {
            continue;
        }

        if (!TF_VERIFY(handle.isAlive())) {
            // We should have removed this handle from all dirty lists before
            // it died. Clean it up now so that this doesn't happen again.
            _StopTrackingInstancer(handle);
            continue;
        }

        const MDagPath firstInstancePath =
                MDagPath::getAPathTo(handle.object());

        // *Don't* create the adapter if it doesn't exist.
        // Logically, it should have already been created.
        _InstancerEntry& entry = iter->second;
        std::unique_ptr<UsdMayaGL_InstancerShapeAdapter>& adapter =
                vp2 ? entry.adapterVp2 : entry.adapterLegacy;
        if (!adapter) {
            TF_CODING_ERROR(
                    "Trying to update xform for '%s' but can't find adapter",
                    firstInstancePath.fullPathName().asChar());
            continue;
        }

        MStatus status;
        const MMatrix transform = firstInstancePath.inclusiveMatrix(&status);
        CHECK_MSTATUS(status);
        adapter->SetRootXform(GfMatrix4d(transform.matrix));
    }
    dirtyInstancerXforms.clear();
}

void
UsdMayaGL_InstancerImager::RemoveShapeAdapters(bool vp2)
{
    UsdMayaGLBatchRenderer& renderer =
            UsdMayaGLBatchRenderer::GetInstance();
    for (auto& handleAndEntry : _instancers) {
        const MObjectHandle& handle = handleAndEntry.first;
        _InstancerEntry& entry = handleAndEntry.second;

        // After deleting the shape adapters, insert the handles into the
        // appropriate dirty queues so that the shape adapters get properly
        // recreated if SyncShapeAdapters() is called again.
        if (vp2 && entry.adapterVp2) {
            renderer.RemoveShapeAdapter(entry.adapterVp2.get());
            entry.adapterVp2.reset();

            _dirtyInstancersVp2.insert(handle);
            _dirtyInstancerXformsVp2.insert(handle);
        }
        else if (!vp2 && entry.adapterLegacy) {
            renderer.RemoveShapeAdapter(entry.adapterLegacy.get());
            entry.adapterLegacy.reset();

            _dirtyInstancersLegacy.insert(handle);
            _dirtyInstancerXformsLegacy.insert(handle);
        }
    }
}

void
UsdMayaGL_InstancerImager::_DirtyHdImagingShape(bool createIfNeeded)
{
    MObject hdImagingShape;
    if (_cachedHdImagingShape.isAlive()) {
        hdImagingShape = _cachedHdImagingShape.object();
    }
    else if (createIfNeeded) {
        hdImagingShape = PxrMayaHdImagingShape::GetOrCreateInstance();
        _cachedHdImagingShape = MObjectHandle(hdImagingShape);
    }

    if (!hdImagingShape.isNull()) {
        MHWRender::MRenderer::setGeometryDrawDirty(hdImagingShape);
    }
}

void
UsdMayaGL_InstancerImager::_StartTrackingInstancer(const MObject& instancer)
{
    MObject nonConstInstancer = instancer;
    MObjectHandle instancerHandle(instancer);
    _InstancerEntry& entry = _instancers[instancerHandle];
    const auto iter = _instancers.find(instancerHandle);

    MDagPath firstInstancePath = MDagPath::getAPathTo(instancer);

    // Note that in the callback below, it is safe to keep a pointer to the
    // key-value pair (&*iter) because pointers into unordered_maps are not
    // invalidated unless the element is erased. And when the element is erased,
    // the callback is removed in the _InstancerEntry's destructor.
    // Note also the peculiar (and seemingly undocumented) behavior of
    // addWorldMatrixModifiedCallback: it listens to world matrix changes on
    // _any instance_, not just the instance specified by firstInstancePath.
    // (That's good in this case!)
    entry.callbacks.append(MDagMessage::addWorldMatrixModifiedCallback(
            firstInstancePath, _OnWorldMatrixChanged, &*iter));
    entry.callbacks.append(MNodeMessage::addNodeDirtyCallback(
            nonConstInstancer, _OnNodeDirty, nullptr));

    TF_DEBUG(PXRUSDMAYAGL_INSTANCER_TRACKING).Msg(
            "Started tracking instancer '%s' (%u)\n",
            firstInstancePath.fullPathName().asChar(),
            instancerHandle.hashCode());

    // Newly-tracked instancers should be marked dirty.
    _dirtyInstancersVp2.insert(instancerHandle);
    _dirtyInstancersLegacy.insert(instancerHandle);
    _dirtyInstancerXformsVp2.insert(instancerHandle);
    _dirtyInstancerXformsLegacy.insert(instancerHandle);
    _DirtyHdImagingShape(true);
}

void
UsdMayaGL_InstancerImager::_StopTrackingInstancer(
    const MObjectHandle& instancerHandle)
{
    auto iter = _instancers.find(instancerHandle);
    if (iter == _instancers.end()) {
        // We're not currently tracking this instancer.
        return;
    }

    // Remove shape adapters from batch renderer.
    _InstancerEntry& entry = iter->second;
    if (entry.adapterVp2) {
        UsdMayaGLBatchRenderer::GetInstance().RemoveShapeAdapter(
                entry.adapterVp2.get());
    }
    if (entry.adapterLegacy) {
        UsdMayaGLBatchRenderer::GetInstance().RemoveShapeAdapter(
                entry.adapterLegacy.get());
    }

    // Remove it from the master list. This will also remove all callbacks.
    _instancers.erase(instancerHandle);

    TF_DEBUG(PXRUSDMAYAGL_INSTANCER_TRACKING).Msg(
            "Stopped tracking instancer (%u)\n",
            instancerHandle.hashCode());

    // Remove it from any dirty lists so that we don't try to sync it again.
    _dirtyInstancersVp2.erase(instancerHandle);
    _dirtyInstancersLegacy.erase(instancerHandle);
    _dirtyInstancerXformsVp2.erase(instancerHandle);
    _dirtyInstancerXformsLegacy.erase(instancerHandle);
}

/* static */
void
UsdMayaGL_InstancerImager::_OnNodeDirty(MObject& node, void* clientData)
{
    UsdMayaGL_InstancerImager& me = GetInstance();
    const MObjectHandle handle(node);

    bool inserted = false;
    inserted |= me._dirtyInstancersVp2.insert(handle).second;
    inserted |= me._dirtyInstancersLegacy.insert(handle).second;
    if (inserted) {
        me._DirtyHdImagingShape(false);
    }
}

/* static */
void
UsdMayaGL_InstancerImager::_OnWorldMatrixChanged(
        MObject &transformNode,
        MDagMessage::MatrixModifiedFlags &modified,
        void *clientData)
{
    UsdMayaGL_InstancerImager& me = GetInstance();
    const auto handleEntryPair =
            static_cast<std::pair<MObjectHandle, _InstancerEntry>*>(clientData);
    const MObjectHandle& handle = handleEntryPair->first;

    bool inserted = false;
    inserted |= me._dirtyInstancerXformsVp2.insert(handle).second;
    inserted |= me._dirtyInstancerXformsLegacy.insert(handle).second;
    if (inserted) {
        me._DirtyHdImagingShape(false);
    }
}

void
UsdMayaGL_InstancerImager::_OnSceneReset(const UsdMayaSceneResetNotice& notice)
{
    TfSingleton<UsdMayaGL_InstancerImager>::DeleteInstance();
    TfSingleton<UsdMayaGL_InstancerImager>::GetInstance();
}

void
UsdMayaGL_InstancerImager::_OnConnection(
        const UsdMayaAssemblyConnectedToInstancerNotice& notice)
{
    if (MGlobal::mayaState() != MGlobal::kInteractive) {
        return;
    }

    MObject instancer = notice.GetInstancer();
    const MObjectHandle instancerHandle(instancer);

    // Remove the instancer from the removal list, if it was previously
    // scheduled for removal.
    _instancersToRemove.erase(instancerHandle);

    // Create a new entry in our instancers list only if we haven't seen this
    // instancer before.
    if (_instancers.count(instancerHandle) == 0) {
        _StartTrackingInstancer(instancer);
    }
}

void
UsdMayaGL_InstancerImager::_OnDisconnection(
        const UsdMayaAssemblyDisconnectedFromInstancerNotice& notice)
{
    if (MGlobal::mayaState() != MGlobal::kInteractive) {
        return;
    }

    MObject instancer = notice.GetInstancer();
    const MObjectHandle instancerHandle(instancer);

    MStatus status;
    MFnDependencyNode instancerDepNode(instancer, &status);
    if (!status) {
        return;
    }

    MPlug inputHierarchy = instancerDepNode.findPlug("inputHierarchy", &status);
    if (!status) {
        return;
    }

    // Check the input hierarchy (prototypes) of the instancer to see if it
    // still requires Hydra drawing.
    const unsigned int numElements = inputHierarchy.numElements();
    for (unsigned int i = 0; i < numElements; ++i) {
        MPlug hierarchyPlug = inputHierarchy[i];
        MPlug source = UsdMayaUtil::GetConnected(hierarchyPlug);
        if (source.isNull()) {
            continue;
        }

        MFnDependencyNode sourceNode(source.node(), &status);
        if (!status) {
            continue;
        }

        if (sourceNode.typeId() == UsdMayaReferenceAssembly::typeId) {
            // There's at least one USD reference assembly still connected to
            // this point instancer, so continue tracking the instancer node.
            return;
        }
    }

    // Queue the instancer for removal. We don't remove it right away because
    // changing prototypes causes instancers to briefly enter states where they
    // have no prototypes (and thus don't need Hydra drawing). Instancers
    // queued for removal will actually be removed on the next invocation of
    // _SyncShapeAdapters().
    _instancersToRemove.insert(instancerHandle);
    _DirtyHdImagingShape(false);
}

UsdMayaGL_InstancerImager::UsdMayaGL_InstancerImager()
{
    TfWeakPtr<UsdMayaGL_InstancerImager> me(this);
    TfNotice::Register(me, &UsdMayaGL_InstancerImager::_OnSceneReset);
    TfNotice::Register(me, &UsdMayaGL_InstancerImager::_OnConnection);
    TfNotice::Register(me, &UsdMayaGL_InstancerImager::_OnDisconnection);
}

UsdMayaGL_InstancerImager::~UsdMayaGL_InstancerImager()
{
    TF_DEBUG(PXRUSDMAYAGL_INSTANCER_TRACKING).Msg(
            "UsdMayaGL_InstancerImager dying; stopped tracking remaining "
            " %zu instancers\n",
            _instancers.size());
}

UsdMayaGL_InstancerImager::_InstancerEntry::~_InstancerEntry()
{
    MMessage::removeCallbacks(callbacks);
}

TF_REGISTRY_FUNCTION(UsdMayaReferenceAssembly)
{
    TfSingleton<UsdMayaGL_InstancerImager>::GetInstance();
}

PXR_NAMESPACE_CLOSE_SCOPE
