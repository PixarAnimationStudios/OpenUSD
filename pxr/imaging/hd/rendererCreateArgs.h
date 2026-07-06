//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDERER_CREATE_ARGS_H
#define PXR_IMAGING_HD_RENDERER_CREATE_ARGS_H

#include "pxr/imaging/hd/rendererCreateArgsSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \deprecated Use HdRendererCreateArgsSchema instead.
///
/// HdRendererCreateArgsSchema has been introduced in HD_API_VERSION 90
/// and HdRendererCreateArgs removed (and replaced by this alias) in
/// HD_API_VERSION 103.
///
using HdRendererCreateArgs = HdRendererCreateArgsSchema;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
