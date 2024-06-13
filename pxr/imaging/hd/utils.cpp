//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/utils.h"

#include "pxr/imaging/hd/materialConnectionSchema.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"
#include "pxr/imaging/hd/materialNetworkSchema.h"
#include "pxr/imaging/hd/materialNodeSchema.h"
#include "pxr/imaging/hd/materialNodeParameterSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdUtils {

/* static */
bool
HasActiveRenderSettingsPrim(
    const HdSceneIndexBaseRefPtr &si,
    SdfPath *primPath /* = nullptr */)
{
    if (!si) {
        return false;
    }

    HdSceneGlobalsSchema sgSchema =
        HdSceneGlobalsSchema::GetFromSceneIndex(si);
    if (!sgSchema) {
        return false;
    }

    if (auto pathHandle = sgSchema.GetActiveRenderSettingsPrim()) {
        const SdfPath rspPath = pathHandle->GetTypedValue(0);
        // Validate prim.
        HdSceneIndexPrim prim = si->GetPrim(rspPath);
        if (prim.primType == HdPrimTypeTokens->renderSettings &&
            prim.dataSource) {
            if (primPath) {
                *primPath = rspPath;
            }
            return true;
        }
    }

    return false;
}

/* static */
bool
GetCurrentFrame(const HdSceneIndexBaseRefPtr &si, double *frame)
{
    if (!si) {
        return false;
    }

    HdSceneGlobalsSchema sgSchema = HdSceneGlobalsSchema::GetFromSceneIndex(si);
    if (!sgSchema) {
        return false;
    }

    if (auto frameHandle = sgSchema.GetCurrentFrame()) {
        const double frameValue = frameHandle->GetTypedValue(0);
        if (std::isnan(frameValue)) {
            return false;
        }
        *frame = frameValue;
        return true;
    }

    return false;
}

CameraUtilConformWindowPolicy
ToConformWindowPolicy(const TfToken &token)
{
    if (token == HdAspectRatioConformPolicyTokens->adjustApertureWidth) {
        return CameraUtilMatchVertically;
    }
    if (token == HdAspectRatioConformPolicyTokens->adjustApertureHeight) {
        return CameraUtilMatchHorizontally;
    }
    if (token == HdAspectRatioConformPolicyTokens->expandAperture) {
        return CameraUtilFit;
    }
    if (token == HdAspectRatioConformPolicyTokens->cropAperture) {
        return CameraUtilCrop;
    }
    if (token == HdAspectRatioConformPolicyTokens->adjustPixelAspectRatio) {
        return CameraUtilDontConform;
    }

    TF_WARN(
        "Invalid aspectRatioConformPolicy value '%s', "
        "falling back to expandAperture.", token.GetText());
    
    return CameraUtilFit;
}

void
PrintSceneIndex(
    std::ostream &out,
    const HdSceneIndexBaseRefPtr &si,
    const SdfPath &rootPath /* = SdfPath::AbsoluteRootPath()*/)
{
    // Traverse the scene index to populate a lexicographically 
    // ordered path set.
    SdfPathSet primPathSet;
    HdSceneIndexPrimView view(si, rootPath);
    for (auto it = view.begin(); it != view.end(); ++it) {
        const SdfPath &primPath = *it;
        primPathSet.insert(primPath);
    }

    // Write out each prim without indenting it based on its depth in the 
    // hierarchy for ease of readability,
    for (const SdfPath &primPath : primPathSet) {
        HdSceneIndexPrim prim = si->GetPrim(primPath);
        if (prim.dataSource) {
            out << "<" << primPath << "> type = " << prim.primType << std::endl;
            
            HdDebugPrintDataSource(out, prim.dataSource, /* indent = */1);
        }
    }
}

HdContainerDataSourceHandle
ConvertHdMaterialNetworkToHdMaterialNetworkSchema(
    const HdMaterialNetworkMap& hdNetworkMap)
{
    HD_TRACE_FUNCTION();

    TfTokenVector terminalsNames;
    std::vector<HdDataSourceBaseHandle> terminalsValues;
    std::vector<TfToken> nodeNames;
    std::vector<HdDataSourceBaseHandle> nodeValues;

    struct ParamData {
        VtValue value;
        TfToken colorSpace;
    };

    for (auto const &iter: hdNetworkMap.map) {
        const TfToken &terminalName = iter.first;
        const HdMaterialNetwork &hdNetwork = iter.second;

        if (hdNetwork.nodes.empty()) {
            continue;
        }

        terminalsNames.push_back(terminalName);

        // Transfer over individual nodes.
        // Note that the same nodes may be shared by multiple terminals.
        // We simply overwrite them here.
        for (const HdMaterialNode &node : hdNetwork.nodes) {
            std::vector<TfToken> paramsNames;
            std::vector<HdDataSourceBaseHandle> paramsValues;

            // Gather parameter value and colorspace metadata in paramsInfo, a 
            // mapping of the parameter name to its value and colorspace data.
            std::map<std::string, ParamData> paramsInfo;
            for (const auto &p : node.parameters) {

                // Strip "colorSpace" prefix 
                const std::pair<std::string, bool> res = 
                    SdfPath::StripPrefixNamespace(p.first, 
                        HdMaterialNodeParameterSchemaTokens->colorSpace);

                // Colorspace metadata
                if (res.second) {
                    paramsInfo[res.first].colorSpace = p.second.Get<TfToken>();
                }
                // Value 
                else {
                    paramsInfo[p.first].value = p.second.Get<VtValue>();
                }
            }

            // Create and store the HdMaterialNodeParameter DataSource
            for (const auto &item : paramsInfo) {
                paramsNames.push_back(TfToken(item.first));
                paramsValues.push_back(
                    HdMaterialNodeParameterSchema::Builder()
                        .SetValue(
                            HdRetainedTypedSampledDataSource<VtValue>::New(
                                item.second.value))
                        .SetColorSpace(
                            item.second.colorSpace.IsEmpty()
                            ? nullptr
                            : HdRetainedTypedSampledDataSource<TfToken>::New(
                                item.second.colorSpace))
                        .Build()
                );
            }

            // Accumulate array connections to the same input
            TfDenseHashMap<TfToken,
                TfSmallVector<HdDataSourceBaseHandle, 8>, TfToken::HashFunctor> 
                    connectionsMap;

            TfSmallVector<TfToken, 8> cNames;
            TfSmallVector<HdDataSourceBaseHandle, 8> cValues;

            for (const HdMaterialRelationship &rel : hdNetwork.relationships) {
                if (rel.outputId == node.path) {
                    TfToken outputPath = rel.inputId.GetToken(); 
                    TfToken outputName = TfToken(rel.inputName.GetString());

                    HdDataSourceBaseHandle c = 
                        HdMaterialConnectionSchema::Builder()
                            .SetUpstreamNodePath(
                                HdRetainedTypedSampledDataSource<TfToken>::New(
                                    outputPath))
                            .SetUpstreamNodeOutputName(
                                HdRetainedTypedSampledDataSource<TfToken>::New(
                                    outputName))
                            .Build();

                    connectionsMap[
                        TfToken(rel.outputName.GetString())].push_back(c);
                }
            }

            cNames.reserve(connectionsMap.size());
            cValues.reserve(connectionsMap.size());

            // NOTE: not const because HdRetainedSmallVectorDataSource needs
            //       a non-const HdDataSourceBaseHandle*
            for (auto &entryPair : connectionsMap) {
                cNames.push_back(entryPair.first);
                cValues.push_back(
                    HdRetainedSmallVectorDataSource::New(
                        entryPair.second.size(), entryPair.second.data()));
            }

            nodeNames.push_back(node.path.GetToken());
            nodeValues.push_back(
                HdMaterialNodeSchema::Builder()
                    .SetParameters(
                        HdRetainedContainerDataSource::New(
                            paramsNames.size(), 
                            paramsNames.data(),
                            paramsValues.data()))
                    .SetInputConnections(
                        HdRetainedContainerDataSource::New(
                            cNames.size(), 
                            cNames.data(),
                            cValues.data()))
                    .SetNodeIdentifier(
                        HdRetainedTypedSampledDataSource<TfToken>::New(
                            node.identifier))
                    .Build());
        }

        terminalsValues.push_back(
            HdMaterialConnectionSchema::Builder()
                .SetUpstreamNodePath(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        hdNetwork.nodes.back().path.GetToken()))
                .SetUpstreamNodeOutputName(
                    HdRetainedTypedSampledDataSource<TfToken>::New(
                        terminalsNames.back()))
                .Build());
    }

    HdContainerDataSourceHandle nodesDefaultContext = 
        HdRetainedContainerDataSource::New(
            nodeNames.size(),
            nodeNames.data(),
            nodeValues.data());

    HdContainerDataSourceHandle terminalsDefaultContext = 
        HdRetainedContainerDataSource::New(
            terminalsNames.size(),
            terminalsNames.data(),
            terminalsValues.data());

    return HdMaterialNetworkSchema::Builder()
        .SetNodes(nodesDefaultContext)
        .SetTerminals(terminalsDefaultContext)
        .Build();
}

HdContainerDataSourceHandle
ConvertHdMaterialNetworkToHdMaterialSchema(
    const HdMaterialNetworkMap &hdNetworkMap)
{
    // Create the material network, potentially one per network selector
    HdDataSourceBaseHandle network = 
        ConvertHdMaterialNetworkToHdMaterialNetworkSchema(hdNetworkMap);

    TfToken defaultContext = HdMaterialSchemaTokens->universalRenderContext;
    return HdMaterialSchema::BuildRetained(
        1, 
        &defaultContext, 
        &network);
}

}

PXR_NAMESPACE_CLOSE_SCOPE
