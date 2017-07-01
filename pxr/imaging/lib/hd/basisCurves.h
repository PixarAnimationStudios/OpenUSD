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
#ifndef HD_BASIS_CURVES_H
#define HD_BASIS_CURVES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdBasisCurvesReprDesc
///
/// Descriptor to configure a drawItem for a repr.
///
struct HdBasisCurvesReprDesc {
    HdBasisCurvesReprDesc(
        HdBasisCurvesGeomStyle geomStyle = HdBasisCurvesGeomStyleInvalid)
        : geomStyle(geomStyle)
        {}

    HdBasisCurvesGeomStyle geomStyle:2;
};

/// Hydra Schema for a collection of curves using a particular basis.
///
class HdBasisCurves : public HdRprim {
public:
    HD_API
    virtual ~HdBasisCurves();

    ///
    /// Topology
    ///
    inline HdBasisCurvesTopology  GetBasisCurvesTopology(HdSceneDelegate* delegate) const;
    inline int                    GetRefineLevel(HdSceneDelegate* delegate)         const;

    /// Configure geometric style of drawItems for \p reprName
    HD_API
    static void ConfigureRepr(TfToken const &reprName,
                              HdBasisCurvesReprDesc desc);

    /// Returns whether refinement is always on or not.
    HD_API
    static bool IsEnabledForceRefinedCurves();
    
protected:
    HD_API
    HdBasisCurves(SdfPath const& id,
                  SdfPath const& instancerId = SdfPath());

    typedef _ReprDescConfigs<HdBasisCurvesReprDesc> _BasisCurvesReprConfig;

    HD_API
    static _BasisCurvesReprConfig::DescArray
        _GetReprDesc(TfToken const &reprName);

private:
    // Class can not be default constructed or copied.
    HdBasisCurves()                                  = delete;
    HdBasisCurves(const HdBasisCurves &)             = delete;
    HdBasisCurves &operator =(const HdBasisCurves &) = delete;

    static _BasisCurvesReprConfig _reprDescConfig;
};

inline HdBasisCurvesTopology
HdBasisCurves::GetBasisCurvesTopology(HdSceneDelegate* delegate) const
{
    return delegate->GetBasisCurvesTopology(GetId());
}

inline int
HdBasisCurves::GetRefineLevel(HdSceneDelegate* delegate) const
{
    return delegate->GetRefineLevel(GetId());
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_BASIS_CURVES_H
