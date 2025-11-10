//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/imaging/hd/dashDotLines.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

HdDashDotLines::HdDashDotLines(SdfPath const& id)
    : HdRprim(id)
{
    /*NOTHING*/
}

HdDashDotLines::~HdDashDotLines() = default;

/* virtual */
TfTokenVector const &
HdDashDotLines::GetBuiltinPrimvarNames() const
{
    static const TfTokenVector primvarNames = {
        HdTokens->points,
        HdTokens->normals,
        HdTokens->widths,
        HdTokens->pattern,
        HdTokens->patternPartCount,
        HdTokens->patternPeriod,
        HdTokens->patternScale,
        HdTokens->startCapType,
        HdTokens->endCapType
    };
    return primvarNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

