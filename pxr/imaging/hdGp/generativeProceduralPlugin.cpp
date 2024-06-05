//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "generativeProceduralPlugin.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<HdGpGenerativeProceduralPlugin>();
}

HdGpGenerativeProceduralPlugin::HdGpGenerativeProceduralPlugin() = default;

//
// As this class is a pure interface class, it does not need a
// vtable.  However, it is possible that some users will use rtti.
// This will cause a problem for some of our compilers:
//
// In particular clang will throw a warning: -wweak-vtables
// For gcc, there is an issue were the rtti typeid's are different.
//
// As destruction of the class is not on the performance path,
// the body of the deleter is provided here, so a vtable is created
// in this compilation unit.
HdGpGenerativeProceduralPlugin::~HdGpGenerativeProceduralPlugin() = default;


/* virtual */
HdGpGenerativeProcedural *
HdGpGenerativeProceduralPlugin::Construct(const SdfPath &proceduralPrimPath)
{
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE