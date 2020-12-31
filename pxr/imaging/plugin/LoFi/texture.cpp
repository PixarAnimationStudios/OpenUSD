//
// Copyright 2020 benmalartre
//
// Unlicensed 
//
#include "pxr/imaging/plugin/LoFi/texture.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"

PXR_NAMESPACE_OPEN_SCOPE


LoFiTexture::LoFiTexture(SdfPath const& id)
  : HdTexture(id)
{
}

LoFiTexture::~LoFiTexture() = default;

void
LoFiTexture::Sync(HdSceneDelegate *sceneDelegate,
                  HdRenderParam   *renderParam,
                  HdDirtyBits     *dirtyBits)
{
    TF_UNUSED(sceneDelegate);
    TF_UNUSED(renderParam);

    *dirtyBits = Clean;
}

HdDirtyBits
LoFiTexture::GetInitialDirtyBitsMask() const
{
    return AllDirty;
}

PXR_NAMESPACE_CLOSE_SCOPE

