//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/api.h"

PXR_NAMESPACE_OPEN_SCOPE

struct ArchAbiBase1 {
    void* dummy;
};

struct ArchAbiBase2 {
    virtual ~ArchAbiBase2() { }
    virtual const char* name() const = 0;
};

template <class T>
struct ArchAbiDerived : public ArchAbiBase1, public ArchAbiBase2 {
    virtual ~ArchAbiDerived() { }
    virtual const char* name() const { return "ArchAbiDerived"; }
};

PXR_NAMESPACE_CLOSE_SCOPE
