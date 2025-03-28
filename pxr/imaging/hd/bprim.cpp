//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/bprim.h"

PXR_NAMESPACE_OPEN_SCOPE

HdBprim::HdBprim(SdfPath const &id)
 : _id(id)
{
}

HdBprim::~HdBprim() = default;

void
HdBprim::Finalize(HdRenderParam *renderParam)
{
}

PXR_NAMESPACE_CLOSE_SCOPE

