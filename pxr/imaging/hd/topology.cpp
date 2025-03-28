//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/topology.h"

PXR_NAMESPACE_OPEN_SCOPE


std::ostream&
operator << (std::ostream &out, HdTopology const &topo)
{
    out << "HdTopology()";
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE

