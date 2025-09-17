//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_BASIS_CURVES_H
#define PXR_IMAGING_HD_BASIS_CURVES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprim.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HD_BASISCURVES_REPR_DESC_TOKENS \
    (surfaceShader)                     \
    (surfaceShaderUnlit)                \
    (hullColor)                         \
    (pointColor)

TF_DECLARE_PUBLIC_TOKENS(HdBasisCurvesReprDescTokens, HD_API,
                         HD_BASISCURVES_REPR_DESC_TOKENS);

/// \class HdBasisCurvesReprDesc
///
/// Descriptor to configure a drawItem for a repr.
///
struct HdBasisCurvesReprDesc
{
    HdBasisCurvesReprDesc(
        HdBasisCurvesGeomStyle geomStyle = HdBasisCurvesGeomStyleInvalid,
        TfToken shadingTerminal = HdBasisCurvesReprDescTokens->surfaceShader)
        : geomStyle(geomStyle),
          shadingTerminal(shadingTerminal)
        {}

    bool IsEmpty() const {
        return geomStyle == HdBasisCurvesGeomStyleInvalid;
    }
    
    HdBasisCurvesGeomStyle geomStyle;
    /// Specifies how the fragment color should be computed from primvar;
    /// this can be used to render heatmap highlighting etc.
    TfToken         shadingTerminal;
};

/// Hydra Schema for a collection of curves using a particular basis.
///
class HdBasisCurves : public HdRprim
{
public:
    HD_API
    ~HdBasisCurves() override;
    
    ///
    /// Topology
    ///
    inline HdBasisCurvesTopology  GetBasisCurvesTopology(HdSceneDelegate* delegate) const;
    inline HdDisplayStyle         GetDisplayStyle(HdSceneDelegate* delegate)        const;

    HD_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;

    /// Configure geometric style of drawItems for \p reprName
    HD_API
    static void ConfigureRepr(TfToken const &reprName,
                              HdBasisCurvesReprDesc desc);

    /// Returns whether refinement is always on or not.
    HD_API
    static bool IsEnabledForceRefinedCurves();
    
protected:
    HD_API
    HdBasisCurves(SdfPath const& id);

    using _BasisCurvesReprConfig = _ReprDescConfigs<HdBasisCurvesReprDesc>;

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

inline HdDisplayStyle
HdBasisCurves::GetDisplayStyle(HdSceneDelegate* delegate) const
{
    return delegate->GetDisplayStyle(GetId());
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_BASIS_CURVES_H
