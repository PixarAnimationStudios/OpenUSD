//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnosticContainer.h"
#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/base/tf/stl.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Tf_DiagnosticContainer::Post()
{
    TfDiagnosticMgr &mgr = TfDiagnosticMgr::GetInstance();
    Iterator it = GetIterator();
    while (it.Next(TfOverloads {
                [&mgr](TfError const &e)   { mgr.PostError(e);    },
                [&mgr](TfWarning const &w) { mgr.PostWarning(w);  },
                [&mgr](TfStatus const &s)  { mgr.PostStatus(s);   }
            }));
    Clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
