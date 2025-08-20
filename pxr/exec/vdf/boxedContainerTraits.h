//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_BOXED_CONTAINER_TRAITS_H
#define PXR_EXEC_VDF_BOXED_CONTAINER_TRAITS_H

#include "pxr/pxr.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
class Vdf_BoxedContainer;

template <typename T>
constexpr inline bool Vdf_IsBoxedContainer = false;

template <typename T>
constexpr inline bool Vdf_IsBoxedContainer<Vdf_BoxedContainer<T>> = true;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
