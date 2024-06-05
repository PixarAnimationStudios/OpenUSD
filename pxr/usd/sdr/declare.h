//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_SDR_DECLARE_H
#define PXR_USD_SDR_DECLARE_H

/// \file sdr/declare.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/declare.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdrShaderNode;
class SdrShaderProperty;

/// Common typedefs that are used throughout the SDR library.

// ShaderNode
typedef SdrShaderNode* SdrShaderNodePtr;
typedef SdrShaderNode const* SdrShaderNodeConstPtr;
typedef std::unique_ptr<SdrShaderNode> SdrShaderNodeUniquePtr;
typedef std::vector<SdrShaderNodeConstPtr> SdrShaderNodePtrVec;

// ShaderProperty
typedef SdrShaderProperty* SdrShaderPropertyPtr;
typedef SdrShaderProperty const* SdrShaderPropertyConstPtr;
typedef std::unique_ptr<SdrShaderProperty> SdrShaderPropertyUniquePtr;
typedef std::unordered_map<TfToken, SdrShaderPropertyConstPtr,
                           TfToken::HashFunctor> SdrPropertyMap;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_DECLARE_H
