//
// Copyright 2019 Pixar
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

#include "hdPrman/gprimbase.h"
#include "hdPrman/rixStrings.h"

PXR_NAMESPACE_OPEN_SCOPE

HdPrman_GprimBase::~HdPrman_GprimBase() = default; 

void
HdPrman_GprimBase::UpdateInstanceVisibility(bool renderPassVisibility,
                                            riley::Riley *riley ) const
{
    if (_renderPassVisibility == renderPassVisibility) {
        return;
    }
    _renderPassVisibility = renderPassVisibility;
    if (!_sceneVisibility) {
        // If the prim is not visible in the scene it cannot be
        // further affected by render pass state.
        return;
    }
    for(auto instid : _instanceIds) {
        RtParamList attrs;
        attrs.SetInteger(RixStr.k_visibility_camera,
                         static_cast<int>(_renderPassVisibility));
        attrs.SetInteger(RixStr.k_visibility_indirect,
                         static_cast<int>(_renderPassVisibility));
        attrs.SetInteger(RixStr.k_visibility_transmission,
                         static_cast<int>(_renderPassVisibility));
        // XXX: HYD-2973: This approach has the unfortunate side-effect
        // of clearing any other attributes that had been previously set
        // on this geometry instance.  This can break features that rely
        // on those attributes, such as subsurface and light-linking.
        riley->ModifyGeometryInstance(
            riley::GeometryPrototypeId::InvalidId(),
            instid, nullptr, nullptr,
            nullptr, &attrs);
    }
}

std::vector<riley::GeometryPrototypeId> 
HdPrman_GprimBase::GetPrototypeIds() const
{
    const std::vector<riley::GeometryPrototypeId> ret(_prototypeIds);
    return ret;
}


PXR_NAMESPACE_CLOSE_SCOPE
