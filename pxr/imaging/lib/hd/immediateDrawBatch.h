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
#ifndef HD_IMMEDIATE_DRAW_BATCH_H
#define HD_IMMEDIATE_DRAW_BATCH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/drawBatch.h"
class HdDrawItemInstance;

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \class Hd_ImmediateDrawBatch
///
/// Drawing batch that is executed immediately.
///
class Hd_ImmediateDrawBatch : public Hd_DrawBatch {
public:
    Hd_ImmediateDrawBatch(HdDrawItemInstance * drawItemInstance);
    virtual ~Hd_ImmediateDrawBatch();

    // Hd_DrawBatch overrides
    virtual bool Validate(bool deepValidation);

    /// Prepare draw commands and apply view frustum culling for this batch.
    virtual void PrepareDraw(HdRenderPassStateSharedPtr const &renderPassState);

    /// Executes the drawing commands for this batch.
    virtual void ExecuteDraw(HdRenderPassStateSharedPtr const &renderPassState);

protected:
    virtual void _Init(HdDrawItemInstance * drawItemInstance);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_IMMEDIATE_DRAW_BATCH_H
