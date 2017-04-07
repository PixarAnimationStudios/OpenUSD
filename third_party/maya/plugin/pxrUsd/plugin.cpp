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
#include <maya/MDrawRegistry.h>
#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/proxyDrawOverride.h"
#include "pxrUsdMayaGL/proxyShapeUI.h"

#include "usdMaya/pluginStaticData.h"
#include "usdMaya/usdImport.h"
#include "usdMaya/usdExport.h"
#include "usdMaya/usdTranslator.h"

PXR_NAMESPACE_USING_DIRECTIVE

static PxrUsdMayaPluginStaticData& _data(PxrUsdMayaPluginStaticData::pxrUsd);

MStatus initializePlugin(
    MObject obj) {

    MStatus status;
    MFnPlugin plugin(obj, "Pixar", "1.0", "Any");


    // we use lambdas to pass in MCreatorFunctions into the various Maya registration
    // functions so that we can specify the static data that the registered
    // shape/data/node should be using.

    status = plugin.registerData(
            _data.stageData.typeName,
            _data.stageData.typeId,
            []() { 
                return UsdMayaStageData::creator(_data.stageData);
            });
    CHECK_MSTATUS(status);

    status = plugin.registerShape(
            _data.proxyShape.typeName,
            _data.proxyShape.typeId,
            []() {
                return UsdMayaProxyShape::creator(_data.proxyShape);  
            },
            []() {
                return UsdMayaProxyShape::initialize(
                    &(_data.proxyShape));
            },
            UsdMayaProxyShapeUI::creator,
            &UsdMayaProxyDrawOverride::sm_drawDbClassification);

    CHECK_MSTATUS(status);

    status = plugin.registerNode(
            _data.referenceAssembly.typeName,
            _data.referenceAssembly.typeId,
            []() {
                return UsdMayaReferenceAssembly::creator(
                    _data.referenceAssembly);
            },
            []() {
                return UsdMayaReferenceAssembly::initialize(
                    &(_data.referenceAssembly));
            },
        MPxNode::kAssembly,
        &UsdMayaReferenceAssembly::_classification);
    CHECK_MSTATUS(status);

    status =
	MHWRender::MDrawRegistry::registerDrawOverrideCreator(
	    UsdMayaProxyDrawOverride::sm_drawDbClassification,
            UsdMayaProxyDrawOverride::sm_drawRegistrantId,
            UsdMayaProxyDrawOverride::Creator);
    CHECK_MSTATUS(status);

    status = MGlobal::sourceFile("usdMaya.mel");
    CHECK_MSTATUS(status);

    // Set the label for the assembly node type so that it appears correctly
    // in the 'Create -> Scene Assembly' menu.
    const MString assemblyTypeLabel("UsdReferenceAssembly");
    MString setLabelCmd;
    status = setLabelCmd.format("assembly -e -type ^1s -label ^2s",
                                _data.referenceAssembly.typeName,
                                assemblyTypeLabel);
    CHECK_MSTATUS(status);
    status = MGlobal::executeCommand(setLabelCmd);
    CHECK_MSTATUS(status);

    // Procs stored in usdMaya.mel
    // Add assembly callbacks for accessing data without creating an MPxAssembly instance
    status = MGlobal::executeCommand("assembly -e -repTypeLabelProc usdMaya_UsdMayaReferenceAssembly_repTypeLabel -type " + _data.referenceAssembly.typeName);
    CHECK_MSTATUS(status);
    status = MGlobal::executeCommand("assembly -e -listRepTypesProc usdMaya_UsdMayaReferenceAssembly_listRepTypes -type " + _data.referenceAssembly.typeName);
    CHECK_MSTATUS(status);

    // Attribute Editor Templates
    // XXX: The try/except here is temporary until we change the Pixar-internal
    // package name to match the external package name.
    MString attribEditorCmd(
        "try:\n"
        "    from pxr.UsdMaya import AEpxrUsdReferenceAssemblyTemplate\n"
        "except ImportError:\n"
        "    from pixar.UsdMaya import AEpxrUsdReferenceAssemblyTemplate\n"
        "AEpxrUsdReferenceAssemblyTemplate.addMelFunctionStubs()");
    status = MGlobal::executePythonCommand(attribEditorCmd);
    CHECK_MSTATUS(status);

    status = plugin.registerCommand("usdExport", 
            usdExport::creator,
            usdExport::createSyntax );
    if (!status) {
        status.perror("registerCommand usdExport");
    }

    status = plugin.registerCommand("usdImport",
            []() { 
                return usdImport::creator(_data.referenceAssembly.typeName.asChar(),
                                          _data.proxyShape.typeName.asChar());
            }, 
            usdImport::createSyntax );

    if (!status) {
        status.perror("registerCommand usdImport");
    }

    // Formerly had separate import/export translators, but due to a bug where
    // referenced .usd files had the wrong type saved (ie, they were saved as
    // pxrUsdExport type, which didn't have a read method!), they are now
    // combined into a single translator.
    // Main downside of this is that the options UI is now less specific - ie,
    // the import options UI will show items which only make sense on export.
    status = plugin.registerFileTranslator("pxrUsd",
                                    "", 
                                    []() { 
                                        return usdTranslator::creator(
                                            _data.referenceAssembly.typeName.asChar(),
                                            _data.proxyShape.typeName.asChar());
                                    }, 
                                    "usdTranslator", // options script name
                                    const_cast<char*>(usdTranslatorDefaults),
                                    false);

    if (!status) {
        status.perror("pxrUsd: unable to register USD translator.");
    }

    return status;
}

MStatus uninitializePlugin(
    MObject obj) {

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

    status = plugin.deregisterFileTranslator("pxrUsd");
    if (!status) {
        status.perror("pxrUsd: unable to deregister USD translator.");
    }

    status = MGlobal::executeCommand("assembly -e -deregister " + _data.referenceAssembly.typeName);
    CHECK_MSTATUS(status);

    status =
	MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(
	    UsdMayaProxyDrawOverride::sm_drawDbClassification,
            UsdMayaProxyDrawOverride::sm_drawRegistrantId);

    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(_data.referenceAssembly.typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterNode(_data.proxyShape.typeId);
    CHECK_MSTATUS(status);

    status = plugin.deregisterData(_data.stageData.typeId);
    CHECK_MSTATUS(status);

    return status;
}
