//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyNanobindBridge.h"

PXR_NAMESPACE_USING_DIRECTIVE

void
wrapTypes(nanobind::module_ &m)
{
    TfNbWrapEnum<PcpArcType>(m);
}
