//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/extGpuImportedBuffer.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSt_ImportedExtGpuBuffer::~HdSt_ImportedExtGpuBuffer()
{
    if (_registry) {
        _registry->_EraseImportedExtGpuBuffer(_key);
    }
    if (_hgi && _handle) {
        _hgi->DestroyBuffer(&_handle);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
