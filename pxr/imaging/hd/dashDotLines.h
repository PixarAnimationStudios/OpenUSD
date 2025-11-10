//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DASH_DOT_LINES_H
#define PXR_IMAGING_HD_DASH_DOT_LINES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Hydra Schema for dashdot styled lines.
///
class HdDashDotLines : public HdRprim
{
public:
    HD_API
    ~HdDashDotLines() override;

    enum DirtyBits : HdDirtyBits {
        DirtyIndices = HdChangeTracker::CustomBitsBegin,
        // Need to update the accumulated length when camera is dirty and the line has screen space
        // Style.
        DirtyCamera = (DirtyIndices << 1)
    };
    
    ///
    /// Topology
    ///
    inline HdDashDotLinesTopology  GetDashDotLinesTopology(HdSceneDelegate* delegate) const;
    inline HdDisplayStyle         GetDisplayStyle(HdSceneDelegate* delegate)        const;

    HD_API
    TfTokenVector const & GetBuiltinPrimvarNames() const override;

protected:
    HD_API
    HdDashDotLines(SdfPath const& id);

private:
    // Class can not be default constructed or copied.
    HdDashDotLines()                                  = delete;
    HdDashDotLines(const HdDashDotLines &)             = delete;
    HdDashDotLines &operator =(const HdDashDotLines &) = delete;
};

inline HdDashDotLinesTopology
HdDashDotLines::GetDashDotLinesTopology(HdSceneDelegate* delegate) const
{
    return delegate->GetDashDotLinesTopology(GetId());
}

inline HdDisplayStyle
HdDashDotLines::GetDisplayStyle(HdSceneDelegate* delegate) const
{
    return delegate->GetDisplayStyle(GetId());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_DASH_DOT_LINES_H
