//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/sprim.h"

PXR_NAMESPACE_OPEN_SCOPE


HdSprim::HdSprim(SdfPath const &id)
 : _id(id)
{
}

HdSprim::~HdSprim() = default;

void
HdSprim::Finalize(HdRenderParam *renderParam)
{
}

PXR_NAMESPACE_CLOSE_SCOPE

