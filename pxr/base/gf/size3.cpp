//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/gf/size3.h"

#include "pxr/base/tf/type.h"

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfSize3>();
}


//  This should probably be moved to ostreamMethods.cpp
//! Output operator
std::ostream &operator<<(std::ostream &o, GfSize3 const &v) {
    return o << "( " << v._vec[0] << " " << v._vec[1] << " " <<
        v._vec[2] << " )";
}

PXR_NAMESPACE_CLOSE_SCOPE
