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
#include "pxr/usd/usdUtils/authoring.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/usd/usd/stage.h"

#include <vector>
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


bool 
UsdUtilsCopyLayerMetadata(const SdfLayerHandle &source,
                          const SdfLayerHandle &destination,
                          bool skipSublayers, 
                          bool bakeUnauthoredFallbacks)
{
    if (!TF_VERIFY(source && destination))
        return false;

    SdfPrimSpecHandle sourcePseudo = source->GetPseudoRoot();
    SdfPrimSpecHandle destPseudo = destination->GetPseudoRoot();
    
    std::vector<TfToken> infoKeys = sourcePseudo->ListInfoKeys();
    std::vector<TfToken>::iterator last = infoKeys.end();
    
    if (skipSublayers){
        last = std::remove_if(infoKeys.begin(), last,
                              [](TfToken key) { return (key == SdfFieldKeys->SubLayers || key == SdfFieldKeys->SubLayerOffsets); });
    }

    for (auto key = infoKeys.begin(); key != last; ++key){
        destPseudo->SetInfo(*key, sourcePseudo->GetInfo(*key));
    }

    if (bakeUnauthoredFallbacks) {
        bool bakeColorConfiguration = 
            std::find(infoKeys.begin(), infoKeys.end(), 
                    SdfFieldKeys->ColorConfiguration) == infoKeys.end();
        bool bakeColorManagementSystem = 
            std::find(infoKeys.begin(), infoKeys.end(), 
                    SdfFieldKeys->ColorManagementSystem) == infoKeys.end();
        
        if (bakeColorConfiguration or bakeColorManagementSystem) {
            SdfAssetPath fallbackColorConfig;
            TfToken fallbackCms;
            
            UsdStage::GetColorConfigFallbacks(&fallbackColorConfig, 
                                              &fallbackCms);

            if (bakeColorConfiguration and 
                !fallbackColorConfig.GetAssetPath().empty()) {
                destPseudo->SetInfo(SdfFieldKeys->ColorConfiguration,
                                    VtValue(fallbackColorConfig));
            }
            if (bakeColorManagementSystem and !fallbackCms.IsEmpty()) {
                destPseudo->SetInfo(SdfFieldKeys->ColorManagementSystem, 
                                    VtValue(fallbackCms));
            }
        }
    }

    return true; 
}


PXR_NAMESPACE_CLOSE_SCOPE

