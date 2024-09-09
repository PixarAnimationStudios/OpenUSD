//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/tf/pyObjectFinder.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/typeInfoMap.h"

using std::type_info;

PXR_NAMESPACE_OPEN_SCOPE

using namespace pxr_boost::python;

static TfStaticData<TfTypeInfoMap<Tf_PyObjectFinderBase const *> > _finders;


void Tf_RegisterPythonObjectFinderInternal(std::type_info const &type,
                                           Tf_PyObjectFinderBase const *finder) {
    _finders->Set(type, finder);
}

object Tf_FindPythonObject(void const *objPtr, std::type_info const &type) {
    Tf_PyObjectFinderBase const *finder = 0;
    if (Tf_PyObjectFinderBase const **x = _finders->Find(type))
        finder = *x;
    if (finder)
        return finder->Find(objPtr);
    return object();
}
    

Tf_PyObjectFinderBase::~Tf_PyObjectFinderBase() {}

PXR_NAMESPACE_CLOSE_SCOPE
