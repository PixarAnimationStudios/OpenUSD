//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/pyModule.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_WRAP_MODULE
{
    TF_WRAP_NB( Notice );
    TF_WRAP_NB( Plugin );    
    TF_WRAP_NB( Registry );    

    TF_WRAP_NB( _TestPlugBase );    
}
