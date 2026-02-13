//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/cacheView.h"

#include "pxr/exec/exec/valueExtractor.h"

#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

Exec_CacheView::Exec_CacheView(
    const VdfDataManagerFacade dataManager,
    TfSpan<const VdfMaskedOutput> outputs,
    TfSpan<const Exec_ValueExtractor> extractors)
    : _dataManager(
        // Set the view to an invalid state if the outputs and extractors
        // don't line up.
        TF_VERIFY(outputs.size() == extractors.size())
        ? std::optional(dataManager)
        : std::nullopt)
    , _outputs(outputs)
    , _extractors(extractors)
{
}

VtValue
Exec_CacheView::Get(int index) const
{
    if (!_dataManager) {
        TF_CODING_ERROR("Cannot extract from invalid view");
        return VtValue();
    }

    if (!(0 <= index && index < static_cast<int>(_outputs.size()))) {
        TF_CODING_ERROR("Index '%d' out of range", index);
        return VtValue();
    }

    const VdfMaskedOutput &mo = _outputs[index];

    // It is allowed for computed values to be empty. This can happen if a leaf
    // node could not be created, because it depends on a cycle.
    if (!mo) {
        return VtValue();
    }

    const VdfVector *v = _dataManager->GetOutputValue(
        *mo.GetOutput(), mo.GetMask());
    if (!v) {
        TF_CODING_ERROR("No value cached for output '%s' (index=%d)",
                        mo.GetDebugName().c_str(), index);
        return VtValue();
    }

    const Exec_ValueExtractor extractor = _extractors[index];
    return extractor(*v, mo.GetMask());
}

PXR_NAMESPACE_CLOSE_SCOPE
