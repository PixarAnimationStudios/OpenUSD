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
#include "usdMaya/primWriter.h"
#include "usdMaya/shaderWriter.h"
#include "usdMaya/shadingModeExporter.h"
#include "usdMaya/shadingModeExporterContext.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/shadingUtil.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"

#include <maya/MFn.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <memory>
#include <string>


PXR_NAMESPACE_OPEN_SCOPE


namespace {

using _NodeHandleToShaderWriterMap =
    UsdMayaUtil::MObjectHandleUnorderedMap<UsdMayaShaderWriterSharedPtr>;

class UseRegistryShadingModeExporter : public UsdMayaShadingModeExporter
{
    public:

        UseRegistryShadingModeExporter() {}

    private:

        /// Gets a shader writer for \p depNode that authors its prim(s) under
        /// the path \p parentPath.
        ///
        /// If no shader writer can be found for the Maya node or if the node
        /// otherwise should not be authored, an empty pointer is returned.
        ///
        /// A cached mapping of node handles to shader writer pointers is
        /// maintained in the provided \p shaderWriterMap.
        UsdMayaShaderWriterSharedPtr
        _GetShaderWriterForNode(
            const MObject& depNode,
            const SdfPath& parentPath,
            const UsdMayaShadingModeExportContext& context,
            _NodeHandleToShaderWriterMap& shaderWriterMap)
        {
            if (depNode.hasFn(MFn::kShadingEngine)) {
                // depNode is the material itself, so we don't need to create a
                // new shader. Connections between it and the top-level shader
                // will be handled by the main Export() method.
                return nullptr;
            }

            if (depNode.hasFn(MFn::kDagNode)) {
                // XXX: Skip DAG nodes for now, but we may eventually want/need
                // to consider them.
                return nullptr;
            }

            const MObjectHandle nodeHandle(depNode);
            const auto iter = shaderWriterMap.find(nodeHandle);
            if (iter != shaderWriterMap.end()) {
                // We've already created a shader writer for this node, so just
                // return it.
                return iter->second;
            }

            // No shader writer exists for this node yet, so create one.
            MStatus status;
            const MFnDependencyNode depNodeFn(depNode, &status);
            if (status != MS::kSuccess) {
                return nullptr;
            }

            const TfToken shaderUsdPrimName(
                UsdMayaUtil::SanitizeName(depNodeFn.name().asChar()));

            const SdfPath shaderUsdPath =
                parentPath.AppendChild(shaderUsdPrimName);

            UsdMayaPrimWriterSharedPtr primWriter =
                context.GetWriteJobContext().CreatePrimWriter(
                    depNodeFn,
                    shaderUsdPath);

            UsdMayaShaderWriterSharedPtr shaderWriter =
                std::dynamic_pointer_cast<UsdMayaShaderWriter>(primWriter);

            // Store the shader writer pointer whether we succeeded or not so
            // that we don't repeatedly attempt and fail to create it for the
            // same node.
            shaderWriterMap[nodeHandle] = shaderWriter;

            return shaderWriter;
        }

        /// Export nodes in the Maya dependency graph rooted at \p rootPlug
        /// for \p material.
        ///
        /// The root plug should be from an attribute on the Maya shadingEngine
        /// node that \p material represents.
        ///
        /// The first shader prim authored during the traversal will be assumed
        /// to be the primary shader for the connection represented by
        /// \p rootPlug. That shader prim will be returned so that it can be
        /// connected to the Material prim.
        UsdShadeShader
        _ExportShadingDepGraph(
                UsdShadeMaterial& material,
                const MPlug& rootPlug,
                const UsdMayaShadingModeExportContext& context)
        {
            // Maintain a mapping of Maya shading node handles to shader
            // writers so that we only author each shader once, but can still
            // look them up again to create connections.
            _NodeHandleToShaderWriterMap shaderWriterMap;

            // MItDependencyGraph takes a non-const MPlug as a constructor
            // parameter, so we have to make a copy of rootPlug here.
            MPlug rootPlugCopy(rootPlug);

            MStatus status;
            MItDependencyGraph iterDepGraph(
                rootPlugCopy,
                MFn::kInvalid,
                MItDependencyGraph::Direction::kUpstream,
                MItDependencyGraph::Traversal::kDepthFirst,
                MItDependencyGraph::Level::kPlugLevel,
                &status);
            if (status != MS::kSuccess) {
                return UsdShadeShader();
            }

            // We'll consider the first shader we create to be the "top-level"
            // shader, which will be the one we return so that it can be
            // connected to the Material prim.
            UsdShadeShader topLevelShader;

            for (; !iterDepGraph.isDone(); iterDepGraph.next()) {
                const MPlug iterPlug = iterDepGraph.thisPlug(&status);
                if (status != MS::kSuccess) {
                    continue;
                }

                // We'll check the source and the destination(s) of the
                // connection to see if we encounter new shading nodes that
                // need to be exported.
                MPlug srcPlug;
                MPlugArray dstPlugs;

                const bool isDestination = iterPlug.isDestination(&status);
                if (status != MS::kSuccess) {
                    continue;
                }
                const bool isSource = iterPlug.isSource(&status);
                if (status != MS::kSuccess) {
                    continue;
                }

                // Note that MPlug's source() and destinations() methods were
                // added in Maya 2016 Extension 2.
                if (isDestination) {
#if MAYA_API_VERSION >= 201651
                    srcPlug = iterPlug.source(&status);
                    if (status != MS::kSuccess) {
                        continue;
                    }
#else
                    MPlugArray srcPlugs;
                    iterPlug.connectedTo(
                        srcPlugs,
                        /* asDst = */ true,
                        /* asSrc = */ false,
                        &status);
                    if (status != MS::kSuccess) {
                        continue;
                    }

                    if (srcPlugs.length() > 0u) {
                        srcPlug = srcPlugs[0u];
                    }
#endif

                    dstPlugs.append(iterPlug);
                } else if (isSource) {
                    srcPlug = iterPlug;

#if MAYA_API_VERSION >= 201651
                    if (!iterPlug.destinations(dstPlugs, &status) ||
                            status != MS::kSuccess) {
                        continue;
                    }
#else
                    iterPlug.connectedTo(
                        dstPlugs,
                        /* asDst = */ false,
                        /* asSrc = */ true,
                        &status);
                    if (status != MS::kSuccess) {
                        continue;
                    }
#endif
                }

                UsdMayaShaderWriterSharedPtr srcShaderWriter;

                if (!srcPlug.isNull()) {
                    srcShaderWriter =
                        _GetShaderWriterForNode(
                            srcPlug.node(),
                            material.GetPath(),
                            context,
                            shaderWriterMap);

                    if (srcShaderWriter) {
                        srcShaderWriter->Write(UsdTimeCode::Default());

                        UsdPrim shaderPrim = srcShaderWriter->GetUsdPrim();
                        if (shaderPrim && !topLevelShader) {
                            topLevelShader = UsdShadeShader(shaderPrim);
                        }
                    }
                }

                for (unsigned int i = 0u; i < dstPlugs.length(); ++i) {
                    const MPlug dstPlug = dstPlugs[i];

                    UsdMayaShaderWriterSharedPtr dstShaderWriter;

                    if (!dstPlug.isNull()) {
                        dstShaderWriter =
                            _GetShaderWriterForNode(
                                dstPlug.node(),
                                material.GetPath(),
                                context,
                                shaderWriterMap);

                        if (dstShaderWriter) {
                            dstShaderWriter->Write(UsdTimeCode::Default());

                            UsdPrim shaderPrim = dstShaderWriter->GetUsdPrim();
                            if (shaderPrim && !topLevelShader) {
                                topLevelShader = UsdShadeShader(shaderPrim);
                            }
                        }
                    }

                    if (srcShaderWriter && dstShaderWriter) {
                        // If we have shader writers for both the source and
                        // the destination, see if we can get the USD shading
                        // properties that the Maya plugs represent so that we
                        // can author the connection in USD.

                        const TfToken srcPlugName =
                            TfToken(context.GetStandardAttrName(srcPlug, false));
                        UsdProperty srcProperty =
                            srcShaderWriter->GetShadingPropertyForMayaAttrName(
                                srcPlugName);

                        const TfToken dstPlugName =
                            TfToken(context.GetStandardAttrName(dstPlug, false));
                        UsdProperty dstProperty =
                            dstShaderWriter->GetShadingPropertyForMayaAttrName(
                                dstPlugName);

                        if (srcProperty && dstProperty) {
                            UsdAttribute srcAttribute =
                                srcProperty.As<UsdAttribute>();
                            if (!srcAttribute) {
                                // The source property is not a UsdAttribute,
                                // or possibly the shader writer did not
                                // author/create it, so we can't do anything
                                // with it.
                            }
                            else if (UsdShadeInput::IsInput(srcAttribute)) {
                                UsdShadeInput srcInput(srcAttribute);

                                UsdShadeConnectableAPI::ConnectToSource(
                                    dstProperty,
                                    srcInput);
                            }
                            else if (UsdShadeOutput::IsOutput(srcAttribute)) {
                                UsdShadeOutput srcOutput(srcAttribute);

                                UsdShadeConnectableAPI::ConnectToSource(
                                    dstProperty,
                                    srcOutput);
                            }
                        }
                    }
                }
            }

            return topLevelShader;
        }

        void
        Export(
                const UsdMayaShadingModeExportContext& context,
                UsdShadeMaterial* const mat,
                SdfPathSet* const boundPrimPaths) override
        {
            MStatus status;

            MObject shadingEngine = context.GetShadingEngine();
            const MFnDependencyNode shadingEngineDepNodeFn(
                shadingEngine,
                &status);
            if (status != MS::kSuccess) {
                TF_RUNTIME_ERROR(
                    "Cannot export invalid shading engine node '%s'\n",
                    UsdMayaUtil::GetMayaNodeName(shadingEngine).c_str());
                return;
            }

            const UsdMayaShadingModeExportContext::AssignmentVector& assignments =
                context.GetAssignments();
            if (assignments.empty()) {
                return;
            }

            UsdPrim materialPrim =
                context.MakeStandardMaterialPrim(
                    assignments,
                    std::string(),
                    boundPrimPaths);
            UsdShadeMaterial material(materialPrim);
            if (!material) {
                return;
            }

            if (mat != nullptr) {
                *mat = material;
            }

            UsdShadeShader surfaceShaderSchema =
                _ExportShadingDepGraph(
                    material,
                    context.GetSurfaceShaderPlug(),
                    context);
            UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
                surfaceShaderSchema,
                UsdShadeTokens->surface,
                SdfValueTypeNames->Token,
                material,
                UsdShadeTokens->surface);

            UsdShadeShader volumeShaderSchema =
                _ExportShadingDepGraph(
                    material,
                    context.GetVolumeShaderPlug(),
                    context);
            UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
                volumeShaderSchema,
                UsdShadeTokens->volume,
                SdfValueTypeNames->Token,
                material,
                UsdShadeTokens->volume);

            UsdShadeShader displacementShaderSchema =
                _ExportShadingDepGraph(
                    material,
                    context.GetDisplacementShaderPlug(),
                    context);
            UsdMayaShadingUtil::CreateShaderOutputAndConnectMaterial(
                displacementShaderSchema,
                UsdShadeTokens->displacement,
                SdfValueTypeNames->Token,
                material,
                UsdShadeTokens->displacement);
        }
};

} // anonymous namespace


TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeExportContext, useRegistry)
{
    UsdMayaShadingModeRegistry::GetInstance().RegisterExporter(
        "useRegistry",
        []() -> UsdMayaShadingModeExporterPtr {
            return UsdMayaShadingModeExporterPtr(
                static_cast<UsdMayaShadingModeExporter*>(
                    new UseRegistryShadingModeExporter()));
        }
    );
}


// XXX: No import support yet...


PXR_NAMESPACE_CLOSE_SCOPE
