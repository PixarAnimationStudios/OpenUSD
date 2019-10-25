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
#include "pxr/pxr.h"
#include "usdMaya/hdImagingShape.h"

#include "usdMaya/blockSceneModificationContext.h"
#include "usdMaya/translatorUtil.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <maya/MBoundingBox.h>
#include <maya/MDGMessage.h>
#include <maya/MDagPath.h>
#include <maya/MDataHandle.h>
#include <maya/MFn.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnSet.h>
#include <maya/MNamespace.h>
#include <maya/MNodeMessage.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>
#include <maya/MUuid.h>
#include <maya/MViewport2Renderer.h>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(
    PXRMAYAHD_DEFAULT_SELECTION_RESOLUTION,
    256,
    "Specifies the default resolution of the draw target used for computing "
    "selections. Note that this must match one of the possible values for "
    "pxrHdImagingShape's \"selectionResolution\" attribute (256, 512, 1024, "
    "2048, or 4096).");

// XXX: Supporting area selections in depth (where an object that is occluded
// by another object in the selection is also selected) currently comes with a
// significant performance penalty if the number of objects grows large, so for
// now it is disabled by default. It can be enabled by default using this env
// setting, and within a Maya session it can be toggled on and off with an
// attribute on the pxrHdImagingShape.
TF_DEFINE_ENV_SETTING(
    PXRMAYAHD_ENABLE_DEPTH_SELECTION,
    false,
    "Enables area selection of objects occluded in depth");


TF_DEFINE_PUBLIC_TOKENS(PxrMayaHdImagingShapeTokens,
                        PXRUSDMAYA_HD_IMAGING_SHAPE_TOKENS);

const MTypeId PxrMayaHdImagingShape::typeId(0x00126402);
const MString PxrMayaHdImagingShape::typeName(
    PxrMayaHdImagingShapeTokens->MayaTypeName.GetText());

// Attributes
MObject PxrMayaHdImagingShape::selectionResolutionAttr;
MObject PxrMayaHdImagingShape::enableDepthSelectionAttr;

namespace {

// Generates a new UUID that is extremely unlikely to clash with the UUID of
// any other node in Maya. These are consistent over a Maya session, so that
// we can find the nodes again, but they are re-generated between different
// Maya runs since we don't write the imaging shape to disk.
static
MUuid
_GenerateUuid()
{
    MUuid uuid;
    uuid.generate();
    return uuid;
}

static const std::string _HdImagingTransformName("HdImaging");
static const std::string _HdImagingShapeName =
    TfStringPrintf("%sShape", _HdImagingTransformName.c_str());
static const MUuid _HdImagingTransformUuid = _GenerateUuid();
static const MUuid _HdImagingShapeUuid = _GenerateUuid();

} // anonymous namespace

/* static */
void*
PxrMayaHdImagingShape::creator()
{
    return new PxrMayaHdImagingShape();
}

/* static */
MStatus
PxrMayaHdImagingShape::initialize()
{
    MStatus status;

    MFnEnumAttribute enumAttrFn;
    MFnNumericAttribute numericAttrFn;

    const int defaultSelectionResolution =
        TfGetEnvSetting(PXRMAYAHD_DEFAULT_SELECTION_RESOLUTION);

    selectionResolutionAttr =
        enumAttrFn.create(
            "selectionResolution",
            "sr",
            defaultSelectionResolution,
            &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = enumAttrFn.addField("256x256", 256);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = enumAttrFn.addField("512x512", 512);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = enumAttrFn.addField("1024x1024", 1024);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = enumAttrFn.addField("2048x2048", 2048);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = enumAttrFn.addField("4096x4096", 4096);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = enumAttrFn.setInternal(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = enumAttrFn.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = enumAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(selectionResolutionAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    const bool enableDepthSelection =
        TfGetEnvSetting(PXRMAYAHD_ENABLE_DEPTH_SELECTION);

    enableDepthSelectionAttr = numericAttrFn.create(
        "enableDepthSelection",
        "eds",
        MFnNumericData::kBoolean,
        0.0,
        &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setDefault(enableDepthSelection);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setInternal(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setStorable(false);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = numericAttrFn.setAffectsAppearance(true);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    status = addAttribute(enableDepthSelectionAttr);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

/* static */
PxrMayaHdImagingShape*
PxrMayaHdImagingShape::GetShapeAtDagPath(const MDagPath& dagPath)
{
    const MObject mObj = dagPath.node();
    if (mObj.apiType() != MFn::kPluginShape) {
        TF_CODING_ERROR(
            "Could not get PxrMayaHdImagingShape for non-plugin shape node at "
            "DAG path: %s (apiTypeStr = %s)",
            dagPath.fullPathName().asChar(),
            mObj.apiTypeStr());
        return nullptr;
    }

    const MFnDependencyNode depNodeFn(mObj);
    PxrMayaHdImagingShape* imagingShape =
        static_cast<PxrMayaHdImagingShape*>(depNodeFn.userNode());
    if (!imagingShape) {
        TF_CODING_ERROR(
            "Could not get PxrMayaHdImagingShape for node at DAG path: %s",
            dagPath.fullPathName().asChar());
        return nullptr;
    }

    return imagingShape;
}

/* static */
MObject
PxrMayaHdImagingShape::GetOrCreateInstance()
{
    MStatus status;

    // Look up the imaging shape via UUID; this is namespace-independent.
    MSelectionList selList;
    selList.add(_HdImagingShapeUuid);

    MObject hdImagingShapeObj;
    if (!selList.isEmpty() && selList.getDependNode(0, hdImagingShapeObj)) {
        return hdImagingShapeObj;
    }

    // Ensure that we create the nodes in the root namespace, in case this
    // function is getting invoked by a node in a non-root namespace (e.g. a USD
    // proxy shape that represents the "Collapsed" representation of an
    // assembly).
    const MString currNamespace = MNamespace::currentNamespace(&status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());
    const MString rootNamespace = MNamespace::rootNamespace(&status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    MNamespace::setCurrentNamespace(rootNamespace);

    // We never intend for the imaging shape to get saved out to the Maya scene
    // file, so make sure that we preserve the scene modification status from
    // before we create the shape.
    const UsdMayaBlockSceneModificationContext blockModContext;

    // Create a transform node for the shape.
    MObject hdImagingTransformObj;
    if (!UsdMayaTranslatorUtil::CreateNode(
            _HdImagingTransformName.c_str(),
            "transform",
            MObject::kNullObj,
            &status,
            &hdImagingTransformObj)) {
        TF_RUNTIME_ERROR("Failed to create transform node %s for %s",
                         _HdImagingTransformName.c_str(),
                         _HdImagingShapeName.c_str());

        MNamespace::setCurrentNamespace(currNamespace);

        return MObject();
    }

    // Set the do not write flag, set its UUID, and hide it in the outliner.
    // Don't lock the transform, because that causes problems reordering root
    // nodes. Do lock all the attributes on the transform to limit possible
    // shenanigans. If there is an error, let Maya report it but keep going.
    MFnDependencyNode depNodeFn(hdImagingTransformObj, &status);
    CHECK_MSTATUS(status);

    status = depNodeFn.setDoNotWrite(true);
    CHECK_MSTATUS(status);

    depNodeFn.setUuid(_HdImagingTransformUuid, &status);
    CHECK_MSTATUS(status);

    UsdMayaUtil::SetHiddenInOutliner(depNodeFn, true);

    for (unsigned int i = 0u; i < depNodeFn.attributeCount(); ++i) {
        const MObject attribute = depNodeFn.attribute(i);
        MPlug plug = depNodeFn.findPlug(attribute, true);
        plug.setLocked(true);
    }

    // Create the HdImagingShape.
    if (!UsdMayaTranslatorUtil::CreateNode(
            _HdImagingShapeName.c_str(),
            PxrMayaHdImagingShapeTokens->MayaTypeName.GetText(),
            hdImagingTransformObj,
            &status,
            &hdImagingShapeObj)) {
        TF_RUNTIME_ERROR("Failed to create %s", _HdImagingShapeName.c_str());

        MNamespace::setCurrentNamespace(currNamespace);

        return MObject();
    }

    // We have to lock the pxrHdImagingShape here as opposed to in the shape's
    // postConstructor(), otherwise the rename CreateNode() above tries to do
    // will fail.
    status = depNodeFn.setObject(hdImagingShapeObj);
    CHECK_MSTATUS(status);

    status = depNodeFn.setLocked(true);
    CHECK_MSTATUS(status);

    MNamespace::setCurrentNamespace(currNamespace);

    return hdImagingShapeObj;
}

/* virtual */
bool
PxrMayaHdImagingShape::isBounded() const
{
    return false;
}

/* virtual */
MBoundingBox
PxrMayaHdImagingShape::boundingBox() const
{
    return MBoundingBox();
}

/* virtual */
void
PxrMayaHdImagingShape::postConstructor()
{
    MStatus status = setDoNotWrite(true);
    CHECK_MSTATUS(status);

    MFnDependencyNode depNodeFn(thisMObject());
    depNodeFn.setUuid(_HdImagingShapeUuid, &status);
    CHECK_MSTATUS(status);

    UsdMayaUtil::SetHiddenInOutliner(depNodeFn, true);
}

/* virtual */
bool
PxrMayaHdImagingShape::getInternalValue(
        const MPlug& plug,
        MDataHandle& dataHandle)
{
    if (plug == selectionResolutionAttr ||
            plug == enableDepthSelectionAttr) {
        // We just want notification of attribute gets and sets. We return
        // false here to tell Maya that it should still manage storage of the
        // value in the data block.
        return false;
    }

    return MPxSurfaceShape::getInternalValue(plug, dataHandle);
}

/* virtual */
bool
PxrMayaHdImagingShape::setInternalValue(
        const MPlug& plug,
        const MDataHandle& dataHandle)
{
    if (plug == selectionResolutionAttr ||
            plug == enableDepthSelectionAttr) {
        // If these attributes are changed, we mark the HdImagingShape as
        // needing to be redrawn, which is when we'll pull the new values from
        // the shape and pass them to the batch renderer.
        MHWRender::MRenderer::setGeometryDrawDirty(thisMObject());

        // We just want notification of attribute gets and sets. We return
        // false here to tell Maya that it should still manage storage of the
        // value in the data block.
        return false;
    }

    return MPxSurfaceShape::setInternalValue(plug, dataHandle);
}

/* static */
void
PxrMayaHdImagingShape::_OnObjectSetAdded(MObject& node, void* clientData)
{
    MStatus status;
    MFnSet objectSet(node, &status);
    if (!status) {
        return;
    }

    // Maya constructs sets with the name <modelPanelName>ViewSelectedSet to
    // track nodes that should be drawn in isolate selection mode.
    // For all practical purposes, we can assume that a set with this suffix
    // is such a Maya-controlled set. (If we wanted to be more robust, we could
    // query the MEL command isolateSelect, but that seems overkill here.)
    if (!TfStringEndsWith(objectSet.name().asChar(), "ViewSelectedSet")) {
        return;
    }

    // We listen to attribute changed callbacks on this set so that we can
    // re-add ourselves if the user changes the set of nodes to isolate without
    // exiting isolate selection mode. If the node is already being tracked,
    // then skip it.
    PxrMayaHdImagingShape* me =
            static_cast<PxrMayaHdImagingShape*>(clientData);
    MObjectHandle handle(node);
    if (me->_objectSetAttrChangedCallbackIds.count(handle) != 0) {
        return;
    }
    me->_objectSetAttrChangedCallbackIds[handle] =
            MNodeMessage::addAttributeChangedCallback(
            node, _OnObjectSetAttrChanged, me);

    // In rare cases, the user may have manually added the pxrHdImagingShape
    // into the isolate selection list. However, we won't know about it until
    // the connection between the shape and the set is made. This isn't a big
    // deal, though, since it's OK for the shape to appear twice in the set.
    objectSet.addMember(me->thisMObject());
}

/* static */
void
PxrMayaHdImagingShape::_OnObjectSetRemoved(MObject& node, void* clientData)
{
    MStatus status;
    MFnSet objectSet(node, &status);
    if (!status) {
        return;
    }

    // Just to be safe, always check the removed set to see if we've been
    // tracking it, regardless of the set's name.
    PxrMayaHdImagingShape* me =
            static_cast<PxrMayaHdImagingShape*>(clientData);
    MObjectHandle handle(node);
    auto iter = me->_objectSetAttrChangedCallbackIds.find(handle);
    if (iter == me->_objectSetAttrChangedCallbackIds.end()) {
        return;
    }

    // Undo everything that we did in _OnObjectSetAdded by removing callbacks
    // and then removing ourselves from the set.
    MMessage::removeCallback(iter->second);
    me->_objectSetAttrChangedCallbackIds.erase(iter);
    objectSet.removeMember(me->thisMObject());
}

/* static */
void
PxrMayaHdImagingShape::_OnObjectSetAttrChanged(
        MNodeMessage::AttributeMessage msg,
        MPlug& plug,
        MPlug& otherPlug,
        void *clientData)
{
    // We only care about the case where the user has loaded a different set of
    // nodes into the isolate selection set, and when that happens, new
    // connections are made with the set. So we only listen for connection-made
    // messages.
    if (!(msg & MNodeMessage::kConnectionMade)) {
        return;
    }

    // If the connection-made message indicates that _this node_ is the node
    // connecting to the set, then there is no more work for us to do, so
    // simply return.
    PxrMayaHdImagingShape* me =
            static_cast<PxrMayaHdImagingShape*>(clientData);
    if (otherPlug.node() == me->thisMObject()) {
        return;
    }

    MFnSet objectSet(plug.node());
    objectSet.addMember(me->thisMObject());
}

PxrMayaHdImagingShape::PxrMayaHdImagingShape() : MPxSurfaceShape()
{
    // If a shape is isolated but depends on Hydra batched drawing for imaging,
    // it won't image in the viewport unless the pxrHdImagingShape is also
    // isolated. This is because Maya skips drawing the pxrHdImagingShape if
    // it's not also isolated, but the pxrHdImagingShape is the one doing the
    // actual drawing for the original shape. Thus, we listen for the
    // addition/removal of objectSets so that we can insert ourselves into any
    // objectSets used for viewport isolate selection.
    _objectSetAddedCallbackId = MDGMessage::addNodeAddedCallback(
            _OnObjectSetAdded, "objectSet", this);
    _objectSetRemovedCallbackId = MDGMessage::addNodeRemovedCallback(
            _OnObjectSetRemoved, "objectSet", this);
}

/* virtual */
PxrMayaHdImagingShape::~PxrMayaHdImagingShape()
{
    MMessage::removeCallback(_objectSetAddedCallbackId);
    MMessage::removeCallback(_objectSetRemovedCallbackId);
    for (const auto& handleAndCallbackId : _objectSetAttrChangedCallbackIds) {
        MMessage::removeCallback(handleAndCallbackId.second);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
