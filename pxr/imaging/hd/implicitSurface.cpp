//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hd/implicitSurface.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdImplicitSurfaceReprDescTokens,
        HD_IMPLICITSURFACE_REPR_DESC_TOKENS);

HdImplicitSurface::HdImplicitSurface(SdfPath const& id)
    : HdRprim(id)
{
    /*NOTHING*/
}

HdImplicitSurface::~HdImplicitSurface() = default;

/* virtual */
TfTokenVector const &
HdImplicitSurface::GetBuiltinPrimvarNames() const
{
    static const TfTokenVector primvarNames{};
    return primvarNames;
}

// static repr configuration
HdImplicitSurface::_ImplicitSurfaceReprConfig HdImplicitSurface::_reprDescConfig;

/* static */
void
HdImplicitSurface::ConfigureRepr(TfToken const &reprName,
                                 HdImplicitSurfaceReprDesc desc)
{
    HD_TRACE_FUNCTION();

    _reprDescConfig.AddOrUpdate(
        reprName, _ImplicitSurfaceReprConfig::DescArray{desc});
}

/* static */
HdImplicitSurface::_ImplicitSurfaceReprConfig::DescArray
HdImplicitSurface::_GetReprDesc(TfToken const &reprName)
{
    return _reprDescConfig.Find(reprName);
}

PXR_NAMESPACE_CLOSE_SCOPE
