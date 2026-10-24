//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/anyWeakPtr.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>

PXR_NAMESPACE_USING_DIRECTIVE

void wrapAnyWeakPtr(nanobind::module_ &)
{

    TfPyContainerConversions::from_python_sequence<
        std::set<TfAnyWeakPtr>,
        TfPyContainerConversions::set_policy>();
}
