//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_ITERATORS_H
#define PXR_EXEC_VDF_ITERATORS_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/maskedIterator.h"
#include "pxr/exec/vdf/weightedIterator.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////////
///
/// Common masked iterators.
///

template<
    class T,
    VdfMaskedIteratorMode maskMode = VdfMaskedIteratorMode::VisitSet>
using VdfMaskedReadIterator =
    VdfMaskedIterator<VdfReadIterator<T>, maskMode>;

template<
    class T,
    VdfMaskedIteratorMode maskMode = VdfMaskedIteratorMode::VisitSet>
using VdfMaskedReadWriteIterator =
    VdfMaskedIterator<VdfReadWriteIterator<T>, maskMode>;


////////////////////////////////////////////////////////////////////////////////
///
/// Common weighted iterators.
///

template<class T>
using VdfWeightedReadIterator =
    VdfWeightedIterator<VdfReadIterator<T>>;

template<class T>
using VdfWeightedReadWriteIterator =
    VdfWeightedIterator<VdfReadWriteIterator<T>>;

template<
    class T,
    VdfMaskedIteratorMode maskMode = VdfMaskedIteratorMode::VisitSet>
using VdfWeightedMaskedReadIterator =
    VdfWeightedIterator<VdfMaskedReadIterator<T, maskMode>>;

template<
    class T,
    VdfMaskedIteratorMode maskMode = VdfMaskedIteratorMode::VisitSet>
using VdfWeightedMaskedReadWriteIterator =
    VdfWeightedIterator<VdfMaskedReadWriteIterator<T, maskMode>>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
