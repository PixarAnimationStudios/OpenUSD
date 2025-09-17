//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_TYPES_H
#define PXR_IMAGING_HDX_TYPES_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/imaging/hio/types.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/dictionary.h"

PXR_NAMESPACE_OPEN_SCOPE


// Struct used to send shader inputs from Presto and send them to Hydra
struct HdxShaderInputs {
    VtDictionary parameters;
    VtDictionary textures;
    VtDictionary textureFallbackValues;
    TfTokenVector attributes;
    VtDictionary metaData;
};

HDX_API
bool operator==(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs);
HDX_API
bool operator!=(const HdxShaderInputs& lhs, const HdxShaderInputs& rhs);
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxShaderInputs& pv);

/// Returns the HioFormat for the given HgiFormat
HDX_API
HioFormat HdxGetHioFormat(HgiFormat hgiFormat);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_TYPES_H
