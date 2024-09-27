//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GPRIMBASE_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GPRIMBASE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/rprim.h"

#include <Riley.h>
#include <RileyIds.h>

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// A common base class for HdPrman_Gprim types
class HdPrman_GprimBase
{
public:
    HdPrman_GprimBase() = default;
    virtual ~HdPrman_GprimBase() = 0;

    std::vector<riley::GeometryPrototypeId> GetPrototypeIds() const;

protected:
    std::vector<riley::GeometryPrototypeId> _prototypeIds;
    std::vector<riley::GeometryInstanceId> _instanceIds;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_GPRIMBASE_H
