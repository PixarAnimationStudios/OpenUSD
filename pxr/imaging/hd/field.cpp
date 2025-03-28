//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/field.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdFieldTokens, HD_FIELD_TOKENS);

HdField::HdField(SdfPath const &id)
 : HdBprim(id)
{
}

HdField::~HdField() = default;

PXR_NAMESPACE_CLOSE_SCOPE
