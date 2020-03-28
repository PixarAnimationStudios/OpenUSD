//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/plugin/LoFi/renderPassState.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"


#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE



LoFiRenderPassState::LoFiRenderPassState()
    : HdRenderPassState()
{
}

LoFiRenderPassState::~LoFiRenderPassState()
{
    /*NOTHING*/
}

void
LoFiRenderPassState::Prepare(
    HdResourceRegistrySharedPtr const &resourceRegistry)
{

}


void
LoFiRenderPassState::Bind()
{

}

void
LoFiRenderPassState::Unbind()
{
   
}

PXR_NAMESPACE_CLOSE_SCOPE
