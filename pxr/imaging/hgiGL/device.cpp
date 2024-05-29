//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"

#include "pxr/imaging/hgiGL/device.h"
#include "pxr/imaging/hgiGL/diagnostic.h"


PXR_NAMESPACE_OPEN_SCOPE

HgiGLDevice::HgiGLDevice()
{
    _activeArena = &_defaultArena;
    HgiGLSetupGL4Debug();
}

HgiGLDevice::~HgiGLDevice()
{
}

void
HgiGLDevice::SubmitOps(HgiGLOpsVector const& ops)
{
    for(HgiGLOpsFn const& f : ops) {
        f();
    }
}

void
HgiGLDevice::SetCurrentArena(HgiGLContextArenaHandle const& arena)
{
    if (arena) {
        _activeArena = arena.Get();
    } else {
        _activeArena = &_defaultArena;
    }
}

uint32_t
HgiGLDevice::AcquireFramebuffer(
    HgiGraphicsCmdsDesc const& desc,
    bool resolved)
{
    return _GetArena()->_AcquireFramebuffer(desc, resolved);
}

void
HgiGLDevice::GarbageCollect()
{
    return _GetArena()->_GarbageCollect();
}

HgiGLContextArena const *
HgiGLDevice::_GetArena() const
{
    return _activeArena;
}
HgiGLContextArena *
HgiGLDevice::_GetArena()
{
    return _activeArena;
}

std::ofstream& operator<<(
    std::ofstream& out,
    const HgiGLDevice& dev)
{                           
    out << *dev._GetArena();
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
