//
// Copyright 2017 Pixar
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
#include "gusd/stageEdit.h"

#include "gusd/USD_Utils.h"

#include <SYS/SYS_Hash.h>


PXR_NAMESPACE_OPEN_SCOPE


bool
GusdStageBasicEdit::Apply(const SdfLayerHandle& layer,
                          UT_ErrorSeverity sev) const
{
    SdfChangeBlock changeBlock;

    UT_ASSERT_P(layer);
    for(const auto& variantsPath : _variants)
        GusdUSD_Utils::SetVariantsFromPath(variantsPath, layer);
    return true;
}


bool
GusdStageBasicEdit::Apply(const UsdStagePtr& stage,
                          UT_ErrorSeverity sev) const
{
    UT_ASSERT_P(stage);

    stage->MuteAndUnmuteLayers(
        _layersToMute, /*unmute*/ std::vector<std::string>());
    return true;
}


size_t
GusdStageBasicEdit::GetHash() const
{
    size_t hash = SYShashRange(_variants.begin(), _variants.end());
    SYShashCombine(hash, SYShashRange(_layersToMute.begin(),
                                      _layersToMute.end()));
    return hash;
}


bool
GusdStageBasicEdit::operator==(const GusdStageEdit& o) const
{
    if(const auto* oPtr = dynamic_cast<const GusdStageBasicEdit*>(&o)) {
        return _variants == oPtr->_variants &&
               _layersToMute == oPtr->_layersToMute;
    }
    return false;
}


void
GusdStageBasicEdit::GetPrimPathAndEditFromVariantsPath(
    const SdfPath& pathWithVariants,
    SdfPath& primPath,
    GusdStageBasicEditPtr& edit)
{
    SdfPath variants;
    GusdUSD_Utils::ExtractPrimPathAndVariants(pathWithVariants,
                                              primPath, variants);
    if(!variants.IsEmpty()) {
        if(!edit)
            edit.reset(new GusdStageBasicEdit);
        edit->GetVariants().append(variants);
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
