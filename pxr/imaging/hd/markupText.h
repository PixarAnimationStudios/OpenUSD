//
// Copyright 2021 Pixar
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
#ifndef PXR_IMAGING_HD_MARKUP_TEXT_H
#define PXR_IMAGING_HD_MARKUP_TEXT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Hydra Schema for markupText primitive.
///
class HdMarkupText : public HdRprim {
public:
    HD_API
    virtual ~HdMarkupText() = default;

    ///
    /// Get the topology for the markupText RPrim.
    ///
    inline HdMarkupTextTopology   GetMarkupTextTopology(HdSceneDelegate* delegate) const;

    ///
    /// Get the display style for the markupText RPrim.
    ///
    inline HdDisplayStyle         GetDisplayStyle(HdSceneDelegate* delegate)        const;

    ///
    /// Get the primvars specified for markupText.
    ///
    HD_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;
    
protected:
    HD_API
    HdMarkupText(SdfPath const& id);

private:
    // Class can not be default constructed or copied.
    HdMarkupText()                                = delete;
    HdMarkupText(const HdMarkupText&)            = delete;
    HdMarkupText&operator =(const HdMarkupText&) = delete;
};

inline HdMarkupTextTopology
HdMarkupText::GetMarkupTextTopology(HdSceneDelegate* delegate) const
{
    return delegate->GetMarkupTextTopology(GetId());
}

inline HdDisplayStyle
HdMarkupText::GetDisplayStyle(HdSceneDelegate* delegate) const
{
    return delegate->GetDisplayStyle(GetId());
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_MARKUP_TEXT_H
