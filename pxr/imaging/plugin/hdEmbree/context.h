//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef HDEMBREE_CONTEXT_H
#define HDEMBREE_CONTEXT_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdEmbree/sampler.h"

#include "pxr/base/gf/matrix4f.h"

#include <embree2/rtcore.h>

class HdRprim;

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdEmbreePrototypeContext
///
/// A small bit of state attached to each bit of prototype geometry in embree,
/// for the benefit of HdEmbreeRenderPass::_TraceRay.
///
struct HdEmbreePrototypeContext
{
    /// A pointer back to the owning HdEmbree rprim.
    HdRprim *rprim;
    /// A name-indexed map of primvar samplers.
    TfHashMap<TfToken, HdEmbreePrimvarSampler*, TfToken::HashFunctor>
        primvarMap;
};

///
/// \class HdEmbreeInstanceContext
///
/// A small bit of state attached to each bit of instanced geometry in embree,
/// for the benefit of HdEmbreeRenderPass::_TraceRay.
///
struct HdEmbreeInstanceContext
{
    /// The object-to-world transform, for transforming normals to worldspace.
    GfMatrix4f objectToWorldMatrix;
    /// The scene the prototype geometry lives in, for passing to
    /// rtcInterpolate.
    RTCScene rootScene;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDEMBREE_CONTEXT_H
