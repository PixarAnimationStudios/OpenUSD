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
#ifndef PXR_IMAGING_HD_SIMPLE_TEXT_H
#define PXR_IMAGING_HD_SIMPLE_TEXT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Hydra Schema for a single line single style text.
///
class HdSimpleText : public HdRprim {
public:
    HD_API
    virtual ~HdSimpleText() = default;

    ///
    /// Get the topology for the simpleText RPrim.
    ///
    inline HdSimpleTextTopology   GetSimpleTextTopology(HdSceneDelegate* delegate) const;

    ///
    /// Get the display style for the simpleText RPrim.
    ///
    inline HdDisplayStyle         GetDisplayStyle(HdSceneDelegate* delegate)        const;

    ///
    /// Get the primvars specified for simpleText.
    ///
    HD_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;
    
protected:
    HD_API
    HdSimpleText(SdfPath const& id);

private:
    // Class can not be default constructed or copied.
    HdSimpleText()                                  = delete;
    HdSimpleText(const HdSimpleText&)             = delete;
    HdSimpleText&operator =(const HdSimpleText&) = delete;
};

inline HdSimpleTextTopology
HdSimpleText::GetSimpleTextTopology(HdSceneDelegate* delegate) const
{
    return delegate->GetSimpleTextTopology(GetId());
}

inline HdDisplayStyle
HdSimpleText::GetDisplayStyle(HdSceneDelegate* delegate) const
{
    return delegate->GetDisplayStyle(GetId());
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_SIMPLE_TEXT_H
