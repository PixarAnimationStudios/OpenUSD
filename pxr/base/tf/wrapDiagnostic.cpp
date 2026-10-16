//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pyEnum.h"


PXR_NAMESPACE_USING_DIRECTIVE


#include "pxr/base/tf/pyNanobindBridge.h"

void wrapDiagnostic(nanobind::module_& m)
{
    TfNbWrapEnum<TfDiagnosticType>(m);

    m.def("InstallTerminateAndCrashHandlers",
          TfInstallTerminateAndCrashHandlers);
}
