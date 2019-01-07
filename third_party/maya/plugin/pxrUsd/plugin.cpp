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
#include "pxr/pxr.h"
#include "pxrUsd/api.h"

#include "pxrUsdMayaGL/hdImagingShapeDrawOverride.h"
#include "pxrUsdMayaGL/hdImagingShapeUI.h"
#include "pxrUsdMayaGL/proxyDrawOverride.h"
#include "pxrUsdMayaGL/proxyShapeUI.h"

#include "usdMaya/diagnosticDelegate.h"
#include "usdMaya/exportCommand.h"
#include "usdMaya/exportTranslator.h"
#include "usdMaya/hdImagingShape.h"
#include "usdMaya/importCommand.h"
#include "usdMaya/importTranslator.h"
#include "usdMaya/listShadingModesCommand.h"
#include "usdMaya/notice.h"
#include "usdMaya/pointBasedDeformerNode.h"
#include "usdMaya/proxyShape.h"
#include "usdMaya/referenceAssembly.h"
#include "usdMaya/stageData.h"
#include "usdMaya/stageNode.h"
#include "usdMaya/undoHelperCommand.h"
#include "usdMaya/listUserAttributeWritersCommand.h"

#include <maya/MDrawRegistry.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MPxNode.h>
#include <maya/MStatus.h>
#include <maya/MString.h>


PXR_NAMESPACE_USING_DIRECTIVE


static const MString _RegistrantId("pxrUsdPlugin");


PXRUSD_API
MStatus
initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "Pixar", "1.0", "Any");

    status = plugin.registerData(
        UsdMayaStageData::typeName,
        UsdMayaStageData::mayaTypeId,
        UsdMayaStageData::creator);
    CHECK_MSTATUS(status);

    status = plugin.registerNode(
        UsdMayaStageNode::typeName,
        UsdMayaStageNode::typeId,
        UsdMayaStageNode::creator,
        UsdMayaStageNode::initialize);
    CHECK_MSTATUS(status);

    status = plugin.registerNode(
        UsdMayaPointBasedDeformerNode::typeName,
        UsdMayaPointBasedDeformerNode::typeId,
        UsdMayaPointBasedDeformerNode::creator,
        UsdMayaPointBasedDeformerNode::initialize,
        MPxNode::kDeformerNode);
    CHECK_MSTATUS(status);

    status = plugin.registerShape(
        UsdMayaProxyShape::typeName,
        UsdMayaProxyShape::typeId,
        UsdMayaProxyShape::creator,
        UsdMayaProxyShape::initialize,
        UsdMayaProxyShapeUI::creator,
        &UsdMayaProxyDrawOverride::drawDbClassification);
    CHECK_MSTATUS(status);

    status = plugin.registerNode(
        UsdMayaReferenceAssembly::typeName,
        UsdMayaReferenceAssembly::typeId,
        UsdMayaReferenceAssembly::creator,
        UsdMayaReferenceAssembly::initialize,
        MPxNode::kAssembly,
        &UsdMayaReferenceAssembly::_classification);
    CHECK_MSTATUS(status);

    status = plugin.registerShape(
        PxrMayaHdImagingShape::typeName,
        PxrMayaHdImagingShape::typeId,
        PxrMayaHdImagingShape::creator,
        PxrMayaHdImagingShape::initialize,
        PxrMayaHdImagingShapeUI::creator,
        &PxrMayaHdImagingShapeDrawOverride::drawDbClassification);
    CHECK_MSTATUS(status);

    status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
        PxrMayaHdImagingShapeDrawOverride::drawDbClassification,
        _RegistrantId,
        PxrMayaHdImagingShapeDrawOverride::creator);
    CHECK_MSTATUS(status);

    status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(
        UsdMayaProxyDrawOverride::drawDbClassification,
        _RegistrantId,
        UsdMayaProxyDrawOverride::Creator);
    CHECK_MSTATUS(status);

    status = plugin.registerDisplayFilter(
        UsdMayaProxyShape::displayFilterName,
        UsdMayaProxyShape::displayFilterLabel,
        UsdMayaProxyDrawOverride::drawDbClassification);
    CHECK_MSTATUS(status);

    status = MGlobal::sourceFile("usdMaya.mel");
    CHECK_MSTATUS(status);

    // Set the label for the assembly node type so that it appears correctly
    // in the 'Create -> Scene Assembly' menu.
    const MString assemblyTypeLabel("UsdReferenceAssembly");
    MString setLabelCmd;
    status = setLabelCmd.format("assembly -e -type ^1s -label ^2s",
                                UsdMayaReferenceAssembly::typeName,
                                assemblyTypeLabel);
    CHECK_MSTATUS(status);
    status = MGlobal::executeCommand(setLabelCmd);
    CHECK_MSTATUS(status);

    // Procs stored in usdMaya.mel
    // Add assembly callbacks for accessing data without creating an MPxAssembly instance
    status = MGlobal::executeCommand("assembly -e -repTypeLabelProc usdMaya_UsdMayaReferenceAssembly_repTypeLabel -type " + UsdMayaReferenceAssembly::typeName);
    CHECK_MSTATUS(status);
    status = MGlobal::executeCommand("assembly -e -listRepTypesProc usdMaya_UsdMayaReferenceAssembly_listRepTypes -type " + UsdMayaReferenceAssembly::typeName);
    CHECK_MSTATUS(status);

    // Attribute Editor Templates
    MString attribEditorCmd(
        "from pxr.UsdMaya import AEpxrUsdReferenceAssemblyTemplate\n"
        "AEpxrUsdReferenceAssemblyTemplate.addMelFunctionStubs()");
    status = MGlobal::executePythonCommand(attribEditorCmd);
    CHECK_MSTATUS(status);

    status = plugin.registerCommand(
        "usdExport",
        UsdMayaExportCommand::creator,
        UsdMayaExportCommand::createSyntax);
    if (!status) {
        status.perror("registerCommand usdExport");
    }

    status = plugin.registerCommand(
        "usdImport",
        UsdMayaImportCommand::creator,
        UsdMayaImportCommand::createSyntax);
    if (!status) {
        status.perror("registerCommand usdImport");
    }

    status = plugin.registerCommand(
        "usdListShadingModes",
        UsdMayaListShadingModesCommand::creator,
        UsdMayaListShadingModesCommand::createSyntax);
    if (!status) {
        status.perror("registerCommand usdListShadingModes");
    }

    status = plugin.registerCommand(
        "usdListUserAttributeWriters",
        usdListUserAttributeWriters::creator);

    if (!status) {
        status.perror("registerCommand usdListUserAttributeWriters");
    }

    status = plugin.registerCommand(
        "usdUndoHelperCmd",
        UsdMayaUndoHelperCommand::creator,
        UsdMayaUndoHelperCommand::createSyntax);
    if (!status) {
        status.perror("registerCommand usdUndoHelperCmd");
    }

    status = plugin.registerFileTranslator(
        "pxrUsdImport",
        "",
        UsdMayaImportTranslator::creator,
        "usdTranslatorImport", // options script name
        const_cast<char*>(UsdMayaImportTranslator::GetDefaultOptions().c_str()),
        false);
    if (!status) {
        status.perror("pxrUsd: unable to register USD Import translator.");
    }

    status = plugin.registerFileTranslator(
        "pxrUsdExport",
        "",
        UsdMayaExportTranslator::creator,
        "usdTranslatorExport", // options script name
        const_cast<char*>(UsdMayaExportTranslator::GetDefaultOptions().c_str()),
        true);
    if (!status) {
        status.perror("pxrUsd: unable to register USD Export translator.");
    }

    UsdMayaSceneResetNotice::InstallListener();
    UsdMayaDiagnosticDelegate::InstallDelegate();

    return status;
}

PXRUSD_API
MStatus
uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj);

    status = plugin.deregisterCommand("usdImport");
    if (!status) {
        status.perror("deregisterCommand usdImport");
    }

    status = plugin.deregisterCommand("usdExport");
    if (!status) {
        status.perror("deregisterCommand usdExport");
    }

    status = plugin.deregisterCommand("usdListShadingModes");
    if (!status) {
        status.perror("deregisterCommand usdListShadingModes");
    }

    status = plugin.deregisterCommand("usdListUserAttributeWriters");
    if (!status) {
        status.perror("deregisterCommand usdListUserAttributeWriters");
    }

    status = plugin.deregisterCommand("usdUndoHelperCmd");
    if (!status) {
        status.perror("deregisterCommand usdUndoHelperCmd");
    }

    status = plugin.deregisterFileTranslator("pxrUsdImport");
    if (!status) {
        status.perror("pxrUsd: unable to deregister USD Import translator.");
    }

    status = plugin.deregisterFileTranslator("pxrUsdExport");
    if (!status) {
        status.perror("pxrUsd: unable to deregister USD Export translator.");
    }

    status = MGlobal::executeCommand("assembly -e -deregister " + UsdMayaReferenceAssembly::typeName);
    CHECK_MSTATUS(status);

    status = plugin.deregisterDisplayFilter(
        UsdMayaProxyShape::displayFilterName);
    CHECK_MSTATUS(status);

    status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
        UsdMayaProxyDrawOverride::drawDbClassification,
        _RegistrantId);
    CHECK_MSTATUS(status);

    status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
        PxrMayaHdImagingShapeDrawOverride::drawDbClassification,
        _RegistrantId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(PxrMayaHdImagingShape::typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(UsdMayaReferenceAssembly::typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(UsdMayaProxyShape::typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(UsdMayaPointBasedDeformerNode::typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(UsdMayaStageNode::typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterData(UsdMayaStageData::mayaTypeId);
    CHECK_MSTATUS(status);

    UsdMayaSceneResetNotice::RemoveListener();
    UsdMayaDiagnosticDelegate::RemoveDelegate();

    return status;
}
