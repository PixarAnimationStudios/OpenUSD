//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifdef PXR_BASE_TF_INSTANTIATE_STACKED_H
#error This file may be included only once in a translation unit (.cpp file).
#endif

#define PXR_BASE_TF_INSTANTIATE_STACKED_H

#include "pxr/pxr.h"
#include "pxr/base/tf/stacked.h"

PXR_NAMESPACE_OPEN_SCOPE

#define TF_INSTANTIATE_STACKED(Derived)                \
    template <>                                        \
    std::atomic<typename Derived::Storage::Type*>      \
    Derived::Storage::value(nullptr)

#define TF_INSTANTIATE_DEFINED_STACKED(Derived)        \
    std::atomic<typename Derived::Storage::Type*>      \
    Derived::Storage::value(nullptr)

PXR_NAMESPACE_CLOSE_SCOPE
