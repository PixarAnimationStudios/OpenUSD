//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/makePyConstructor.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Tf_MakePyConstructor {

bp::object _DummyInit(bp::tuple const & /* args */,
                      bp::dict const & /* kw */) {
    return bp::object();
}

}

PXR_NAMESPACE_CLOSE_SCOPE
