//
// Copyright 2023 Pixar
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

#include "pxr/imaging/hd/utils.h"

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
        if (primPath) {
            *primPath = pathHandle->GetTypedValue(0);
        }
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


}

PXR_NAMESPACE_CLOSE_SCOPE
