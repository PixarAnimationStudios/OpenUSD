//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_UNIT_TEST_NULL_RENDER_PASS_H
#define PXR_IMAGING_HD_UNIT_TEST_NULL_RENDER_PASS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/renderPass.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class Hd_UnitTestNullRenderPass
/// Implements the sync part of the render pass, but not the draw part, for
/// core hydra unit tests.
class Hd_UnitTestNullRenderPass : public HdRenderPass
{
public:
    Hd_UnitTestNullRenderPass(HdRenderIndex *index,
                              HdRprimCollection const &collection)
        : HdRenderPass(index, collection)
        {}
    virtual ~Hd_UnitTestNullRenderPass() {}

    void _Execute(HdRenderPassStateSharedPtr const &renderPassState,
                  TfTokenVector const &renderTags) override {}
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_UNIT_TEST_NULL_RENDER_PASS_H
