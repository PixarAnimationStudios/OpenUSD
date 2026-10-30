//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/base/tf/iterator.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include <nanobind/stl/vector.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static VtDictionary
_ReturnDictionary(VtDictionary const &x) {
    return x;
}

static std::vector<VtDictionary>
_DictionaryArrayIdent(std::vector<VtDictionary> const &v) {
    return v;
}

} // anonymous namespace 

void wrapDictionary(nanobind::module_& m)
{
    m.def("_ReturnDictionary", _ReturnDictionary);
    m.def("_DictionaryArrayIdent", _DictionaryArrayIdent);
}
